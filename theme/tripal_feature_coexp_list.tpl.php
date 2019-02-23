<?php
$feature = $variables['node']->feature;

if ($feature->type_id->name == 'gene') {

	// get type_id of coexpression
	$values = array('cv_id' => array('name'=>'sequence'), 'name' => 'associated_with');
    $term = chado_select_record('cvterm', array('cvterm_id', 'name'), $values, array('return_array'=>1));
    $type_id = $term[0]->cvterm_id;

	// search coexpression gene
	$sql = "SELECT R.*, F1.name AS sname, F2.name AS oname FROM chado.feature_relationship R
		LEFT JOIN chado.feature F1 ON F1.feature_id = R.subject_id
		LEFT JOIN chado.feature F2 ON F2.feature_id = R.object_id
		WHERE (R.subject_id = :subject_id OR R.object_id = :object_id) AND R.type_id = :type_id
		ORDER BY R.value DESC";
	$args = array(
		':subject_id' => $feature->feature_id, 
		':object_id' => $feature->feature_id, 
		':type_id' => $type_id
	);
      
	$results = db_query($sql, $args)->fetchAll();

	// build coexpression tables 
	$headers = array('Gene', 'r value', 'Expression');
	$rows = array();
	foreach ($results as $r) {
		$slink = l($r->sname, 'feature/gene/' . $r->sname, array('attributes' => array('target' => "_blank")));
		$olink = l($r->oname, 'feature/gene/' . $r->oname, array('attributes' => array('target' => "_blank")));
		
		$coexp_gene = $slink;
		if ($r->sname == $feature->name) {
			$coexp_gene = $olink;
		}
		$barchart = l('Barchart', 'coexp/'.$r->feature_relationship_id, array('attributes' => array('target' => "_blank")));
		$exptable = l('Table', 'coexptb/'.$r->feature_relationship_id, array('attributes' => array('target' => "_blank")));
		$rows[] = array($coexp_gene, $r->value, $barchart." | ".$exptable);
	}

    $table = array(
      'header' => $headers,
      'rows' => $rows,
      'attributes' => array(
        'id' => 'tripal_coexp-table-properties',
        'class' => 'tripal-data-table table'
      ),
      'sticky' => FALSE,
      'caption' => '',
      'colgroups' => array(),
      'empty' => '',
    );

	print '<div class="row"><div class="col-md-8">';
    print theme_table($table);
	print '</div></div>';
}

