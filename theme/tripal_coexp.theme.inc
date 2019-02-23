<?php
/**
retrieve gene expression by feature id
*/
function tripal_coexp_get_exp($feature_id) {
  // retrieve RNASeq expression dataset for the feature
  $sql = "SELECT EXP.value,
      C.name as type_name,
      P.project_id, P.name as project_name,
      S.biomaterial_id, S.name as biosample_name,
      E.experiment_id, E.name as sample_name
    FROM chado.feature_expressionprop EXP
    LEFT JOIN chado.feature_expression EX ON EXP.feature_expression_id = EX.feature_expression_id
    LEFT JOIN chado.cvterm C ON EXP.type_id = C.cvterm_id
    LEFT JOIN chado.experiment E ON EX.experiment_id = E.experiment_id
    LEFT JOIN chado.biomaterial S ON E.biomaterial_id = S.biomaterial_id
    LEFT JOIN chado.project P ON E.project_id = P.project_id
    WHERE
      EX.feature_id = :feature_id
  ";
  $args = array(':feature_id' => $feature_id);
  $exp = db_query($sql, $args)->fetchAll();
  return $exp;
}

function tripal_coexp_rel_exp($feature_rel_id) {
  
  // get feature ids of two co-express genes
  $sql = "SELECT R.*, F1.name AS sname, F2.name AS oname FROM chado.feature_relationship R
        LEFT JOIN chado.feature F1 ON F1.feature_id = R.subject_id
        LEFT JOIN chado.feature F2 ON F2.feature_id = R.object_id
        WHERE R.feature_relationship_id = :feature_rel_id";
  $args = array(':feature_rel_id' => $feature_rel_id);
  $rel = db_query($sql, $args)->fetchAll();

  $sub_id = $rel[0]->subject_id;
  $obj_id = $rel[0]->object_id;
  $sub_name = $rel[0]->sname;
  $obj_name = $rel[0]->oname;
  $rvalue = $rel[0]->value;

  // retrieve RNASeq expression dataset for these features
  $s_exp = tripal_coexp_get_exp($sub_id);
  $o_exp = tripal_coexp_get_exp($obj_id);

  /**
   * data structure;
   * rnaseq_exp [array]
   *   project_id [obj]
   *     name  = project_name
   *     RPM [array]
   *       sample_id1 = [value (feature1), value (feature2)]
   *       sample_id2 = [value (feature1), value (feature2)]
   *       ...
   *     RPKM [array]
   *     raw_count [array]
   *     [other project properties]
   *      
   *     design [obj]

   * samples [array] 
   *   sample_id [obj]
   *     name = sample_name
   *     factor1 = factor1_name    eg: treatment = Ethylene
   *     factor2 = factor2_name    eg: stage = 10DAA
   *     factorN = factorN_name    eg: tissue = leaf
   *     [other sample properties]
   */
    $rnaseq_exp = array(); // key project id, 
    $samples = array();  // key sample id,
    $samples_name_id = array();
    foreach ($s_exp as $r) {
      $type = $r->type_name;
      $project_id = $r->project_id;
      $project_name = $r->project_name;
      $type = $r->type_name;
      $sample_id = $r->experiment_id;
      $sample_name = $r->sample_name;
      $value = $r->value;

      if (!array_key_exists($project_id, $rnaseq_exp)) {
        $rnaseq_exp[$project_id] = new stdClass;
        if (!isset($rnaseq_exp[$project_id]->name)) {
          $rnaseq_exp[$project_id]->name = $project_name;
          $rnaseq_exp[$project_id]->project_id = $project_id;
        }
      }

      if (!property_exists($rnaseq_exp[$project_id], $type)) {
        $rnaseq_exp[$project_id]->$type = array();
      }
      $rnaseq_exp[$project_id]->$type[$sample_id] = array();
	  $rnaseq_exp[$project_id]->$type[$sample_id][] = $value;

      if (!array_key_exists($sample_id, $samples)) {
        $samples[$sample_id] = new stdClass;
        if (!isset($samples[$sample_id]->name)) {
          $samples[$sample_id]->name = $sample_name;
          $samples_name_id[$sample_name] = $sample_id;
        }
      }
    }
    //end for s_exp, start for o_exp
    foreach ($o_exp as $r) {
      $type = $r->type_name;
      $project_id = $r->project_id;
      $project_name = $r->project_name;
      $type = $r->type_name;
      $sample_id = $r->experiment_id;
      $sample_name = $r->sample_name;
      $value = $r->value;
      $rnaseq_exp[$project_id]->$type[$sample_id][] = $value;
    }
    //end for o_exp

    // retrieve experiment design for each project
    foreach ($rnaseq_exp as $project_id) {
      $values =  array(
        'project_id' => $project_id->project_id,
        'type_id' => array (
          'cv_id' => array (
             'name' => 'bioproject_property',
           ),
           'name' => 'experimental_design',
           'is_obsolete' => 0
        ),
      );
      $result = chado_select_record('projectprop', array('value'), $values);
      if (!empty($result)) {
        $description = array();
        $lines = explode("\n", $result[0]->value);
        $title = array();
        if (preg_match("/^#/", $lines[0])) {
        } else {
          $title = explode(" ", preg_replace("!\s+!", " ", array_shift($lines)));
        } 

        foreach ($lines as $line) {
          if (preg_match("/^#/", $line)) {
            $description[] = $line;
            continue;
          }

          $member = explode(" ", preg_replace("!\s+!", " ", $line));
          $sample_name = $member[0];
          $sample_id = $samples_name_id[$sample_name];

          for($i=1; $i<count($member); ++$i) {
            $member[$i] = trim($member[$i]);
		    if (empty($member[$i])) { 
              continue;
            }
            if (isset($member[$i]) and isset($title[$i])) {
              $factor = $title[$i];
              $factor_name = $member[$i];
              $samples[$sample_id]->$factor = $factor_name;
            }
          }
        }

        // generate designs according to factors
        $factors = $title; 
        array_shift($factors);
        $designs = array();
        $design_num = 0;
        foreach ($factors as $f1) {
          foreach ($factors as $f2) {
            if ($f1 == $f2) {
              continue;
            }
            $design_num++;
            $designs[$design_num] = array($f1, $f2);
          }
        }
			
        if (sizeof($factors) > 0) {
          $rnaseq_exp[$project_id->project_id]->designs = $designs;
        }
        if (sizeof($description) > 0) {
          $rnaseq_exp[$project_id->project_id]->desc = $description;
        }
      }
    }

    // convert the experiments and rnaseq_exp to json format
    $coexp_data = [$sub_name, $obj_name, $rvalue];

	return [$coexp_data, $samples, $rnaseq_exp];
}

function tripal_coexp_preprocess_tripal_feature_coexp_exp(&$variables) {

  // set js path
  $path = drupal_get_path('module', 'tripal_coexp');
  drupal_add_js($path . '/theme/js/d3.min.js', array('type'=>'file'));
  drupal_add_js($path . '/theme/js/plotly-latest.min.js', array('type'=>'file'));

  // get feature rel id of two co-express genes
  $feature_rel_id = $variables['feature_rel_id'];

  // get data by feature rel id
  $data = tripal_coexp_rel_exp($feature_rel_id);
    
  // convert the experiments and rnaseq_exp to json format
  $coexp_data = $data[0]; 
  $samples = $data[1];
  $rnaseq_exp = $data[2];

  $experiments_json = json_encode($samples);
  $rnaseq_exps_json = json_encode($rnaseq_exp);
  $coexp_json = json_encode($coexp_data);

  $data_js = "
	var experiments = $experiments_json;
	var rnaseq_exps = $rnaseq_exps_json;
	var coexp_data = $coexp_json;
  ";
  drupal_add_js($data_js, array('type'=>'inline', 'scope'=>'footer','weight' => 50));
  drupal_add_js($path . '/theme/js/tripal_coexp_barchart_plotly.js', array('type'=>'file', 'scope'=>'footer','weight' => 100));
}

function tripal_coexp_preprocess_tripal_feature_coexp_tab(&$variables) {

  // set js path
  $path = drupal_get_path('module', 'tripal_coexp');

  // get feature rel id of two co-express genes
  $feature_rel_id = $variables['feature_rel_id'];

  // get data by feature rel id
  $data = tripal_coexp_rel_exp($feature_rel_id);

  // convert the experiments and rnaseq_exp to json format
  $coexp_data = $data[0];
  $samples = $data[1];
  $rnaseq_exp = $data[2];

  $variables['coexp_data'] = $coexp_data;
  $variables['samples'] = $samples;
  $variables['rnaseq_exp'] = $rnaseq_exp;
}
