<?php

/**
 * load Co-Expression file into database 
 */ 

/**
 * submit a job for load co-experession file 
 * 
 * @param $node
 * 
 * generate job args, then trigger a tripal job
 */
function tripal_coexp_load_job($node)
{
  global $user;
  if ($node->coexp_name) {

	$values = array('cv_id' => array('name'=>'sequence'), 'name' => 'associated_with');
  	$term = chado_select_record('cvterm', array('cvterm_id', 'name'), $values, array('return_array'=>1));
	$type_id = $term[0]->cvterm_id;

    $job_args = array(
      $node->coexp_path,
	  $type_id
    );
    tripal_add_job("Load Co-Expression File: $node->coexp_path", 'tripal_coexp',
      'tripal_coexp_loader', $job_args, $user->uid);
  }
}

/**
 * loading co-expression
 */
function tripal_coexp_loader($coexpression_file, $type_id) {

  $transaction = db_transaction();
  print "\nNOTE: Loading co-expression is performed using a database transaction. \n" .
     "If the load fails or is terminated prematurely then the entire set of \n" .
     "insertions/updates is rolled back and will not be found in the database\n\n";
  try {
    if (!is_readable($coexpression_file)) {
      watchdog('tripal_coexp_loader',
        'Could not open the file :coexpression_file. Check that file exists and that permissions are correct .',
        array(':coexpression_file' => $coexpression_file),
        WATCHDOG_ERROR);
      exit;
    }

    // parse co-expression file and load to feature_relationship table
    $num_seq = 0;
    $num_insert = 0;
    $num_update = 0;

    // the coexp file do not have title, the format of coexp file:
	// subject_id  object_id  value
    $fh = fopen($coexpression_file, 'r');
    while($line = fgetcsv($fh, 0, "\t")) 
    {
      // check subject and object name, must be exist feature in current database
      if (sizeof($line) != 3) {
		watchdog('tripal_coexp_loader',
          'The format of co-expression file :coexpression_file has error', array(':coexpression_file' => $coexpression_file), WATCHDOG_ERROR);
        exit;
      }

      $subject_name = $line[0];
	  $object_name = $line[1];
	  $value = $line[2];
      $sr = chado_select_record('feature', array('feature_id'), array('uniquename' => $subject_name));
	  $or = chado_select_record('feature', array('feature_id'), array('uniquename' => $object_name));
      if (sizeof($sr) != 1) {
        watchdog('tripal_coexp_loader',
          'Can not find subject feature (Gene/mRNA) :subject from chado', array(':subject' => $subject_name), WATCHDOG_ERROR);
        exit;
      }
	  if (sizeof($or) != 1) {
        watchdog('tripal_coexp_loader',
          'Can not find object feature (Gene/mRNA) :object from chado', array(':object' => $object_name), WATCHDOG_ERROR);
        exit;
      }
	  $subject_id = $sr[0]->feature_id;
	  $object_id = $or[0]->feature_id;
      // check if the relationship is exist 
     
      $sql = "SELECT feature_relationship_id FROM chado.feature_relationship
          WHERE subject_id = :subject_id AND object_id = :object_id AND type_id = :type_id";
      $args = array(':subject_id' => $subject_id, ':object_id' => $object_id, ':type_id' => $type_id);
      $ra = db_query($sql, $args)->fetchAll();
      $args = array(':subject_id' => $object_id, ':object_id' => $subject_id, ':type_id' => $type_id);
	  $rb = db_query($sql, $args)->fetchAll();

	  // update the relationship 
	  if (sizeof($ra) == 1) {
		$update_value = array(
			'value' => $value, //this value is correlation value
		);
		$update = db_update('chado.feature_relationship')
			->fields($update_value)
			->condition('feature_relationship_id', $ra[0]->feature_expression_id)
			->execute();
            
		if (!$update) {
			watchdog('tripal_coexp_loader',
              'Co-expression : can not update record :feature_relationship_id', array(':feature_relationship_id' => $ra[0]->feature_expression_id), WATCHDOG_ERROR);
			exit;
		}

		++$num_update;
      }
      elseif ( sizeof($rb) == 1) {
		$update_value = array( 
            'value' => $value, //this value is correlation value
        );
        $update = db_update('chado.feature_relationship')
            ->fields($update_value)
            ->condition('feature_relationship_id', $rb[0]->feature_expression_id)
            ->execute();

        if (!$update) {
            watchdog('tripal_coexp_loader',
              'Co-expression : can not update record :feature_relationship_id', array(':feature_relationship_id' => $rb[0]->feature_expression_id), WATCHDOG_ERROR);
            exit;
        }

        ++$num_update;
      }
      // insert the relationship
      else {
		$values = array(
            'subject_id' => $subject_id,
            'object_id' => $object_id,
            'type_id' => $type_id,
			'value' => $value,
		);
		$feature_rel_id = db_insert('chado.feature_relationship')->fields($values)->execute();
		++$num_insert;
      }
        
	  // report insert status
      ++$num_seq;
      print  "Insert coexp: " . $num_insert . ", Update coexp: " . $num_update .
      	", coexp " . $num_seq . ". Memory: " .
		number_format(memory_get_usage()) . " bytes.\r";
    }
    fclose($fh);
  }
  catch (Exception $e) {
    print "\n";
    watchdog_exception('tripal_coexp_loader', $e);
    $transaction->rollback();
    print "FAILED: Rolling back database changes...\n";
  }
  print "\nDone.\n";
  return;
}

