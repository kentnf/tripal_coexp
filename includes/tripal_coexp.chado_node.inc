<?php
/**
 * @file
 * Implements the co-expression content type of node. 
 */

/**
 * Implements hook_node_info().
 *
 *  Provides information to drupal about the node types that we're creating
 *  in this module. 
 *
 * @ingroup tripal_coexp
 */
function tripal_coexp_node_info() {
  $nodes = array();
  $nodes['chado_coexp'] = array(
    'name' => t('Coexpression'),
    'base' => 'chado_coexp',
    'description' => t('Load co-expressed gene generated from RNASeq.'),
    'has_title' => TRUE,
    'locked' => TRUE,
    'chado_node_api' => array(
      'base_table' => 'organism',
      'hook_prefix' => 'chado_coexp',
      'linking_table' => 'chado_coexp',
      'record_type_title' => array(
        'singular' => t('coexp'),
        'plural' => t('coexp'),
      ),
    )
  );
  return $nodes;
}

/**
 * Implements hook_node_access().
 * 
 * @param $node
 * Tee node on which the operation is performed, or, if it does not yet exist, the 
 * type of node to be created.
 *
 * @param $op
 * The operation to be performed. 
 * 
 * @param $account
 * A user object representing the user for whom the operation is to be performed. 
 *
 * @return 
 * If the permission for the specified operation is not set then return FALSE. If the
 * permissions is set then return NULL as this allwos other modules to disable
 * access. The only exception is when the $op == 'create'. We will always
 * return TRUE if the permission is set. 
 * 
 * @ingroup tripal_coexp
 * 
 */
function chado_coexp_node_access($node, $op, $account) {
  $node_type = $node;
  if (is_object($node)) {
    $node_type = $node->type;
  }

  if ($node_type == 'chado_coexp') {
    if ($op == 'create') {
      if (!user_access('create chado_coexp content', $account)) {
        return NODE_ACCESS_DENY;
      }
      return NODE_ACCESS_ALLOW;
    }
    if ($op == 'update') {
      if (!user_access('edit chado_coexp content', $account)) {
        return NODE_ACCESS_DENY;
      }
    }
    if ($op == 'delete') {
      if (!user_access('delete chado_coexp content', $account)) {
        return NODE_ACCESS_DENY;
      }
    }
    if ($op == 'view') {
      if (!user_access('access chado_coexp content', $account)) {
        return NODE_ACCESS_DENY;
      }
    }
    return NODE_ACCESS_IGNORE;
  }
}

/**
 * Implements hook_form()
 * 
 * @ingroup tripal_coexp
 */
function chado_coexp_form($node, $form_state) {
  $form = array();

  $or = chado_select_record('feature', array('feature_id'), array('uniquename' => 'Spo00018'));
  dpm($or[0]->feature_id);

  // Not sure what this is for. Is this line necessary? 
  $form['#attribute']['enctype'] = 'multipart/form-data';
 
  // === get default value for the form ===
  if (property_exists($node, 'coexp')) {   
    $coexp = $node->coexp;
    $coexp_name = property_exists($node, 'coexp_name') ? $node->coexp_name : $coexp->name;
    $coexp_path = property_exists($node, 'coexp_path') ? $node->coexp_path : $coexp->path;
    $organism_id = property_exists($node, 'organism_id') ? $node->organism_id : $coexp->organism_id;
  }
  else {
    $coexp_name = property_exists($node, 'coexp_name') ? $node->coexp_name : '';
    $coexp_path = property_exists($node, 'coexp_path') ? $node->coexp_path : '';
    $organism_id = property_exists($node, 'organism_id') ? $node->organism_id : '';
  }
 
  $sql = 'SELECT organism_id, genus, species, common_name FROM {organism} ORDER BY genus';
  $results = chado_query($sql);
  $organisms = array();
  while ($organism = $results->fetchObject()) {
    $organisms[$organism->organism_id] = $organism->common_name;
  }

  $form['organism_id'] = array(
    '#title' => t('Organism'),
    '#type' => 'select',
    '#description' => t('Select the organism, the feature must match the selected orgianism.'),
    '#default_value' => $organism_id,
    '#options' => $organisms,
    '#required' => TRUE,
  );

  // co-expression file path 
  $form['coexp_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name of the co-expression File'),
    '#required' => TRUE,
    '#default_value' =>  $coexp_name,
  );
  $form['coexp_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path of the co-expression File'),
    '#required' => TRUE,
    '#default_value' => $coexp_path,
  );

  $form['coexp_load'] = array(
    '#type' => 'checkbox',
    '#title' => t('Load/Update co-expressed gene for selected organism'),
    '#default_value' => '',
  );

  return $form;
}

/**
 * Implements hook_validate().
 * 
 * @ingroup tripal_coexp
 */
function chado_coexp_validate($node, $form, &$form_state) {
  // check if the term exist
  $values = array('cv_id' => array('name'=>'sequence'), 'name' => 'associated_with');
  $results = chado_select_record('cvterm', array('cvterm_id', 'name'), $values, array('return_array'=>1));
  if (sizeof($results) != 1) {
	form_set_error('coexp_load', t("Can not find term associated_with to load co-expression data."));
  }

  // Don't validate if a delete.
  if ($node->op == 'Delete') {
    return;
  }

  // We are syncing if there is a coexp node but no node ID. In this case a 
  // validation is not required.  
  if (is_null($node->nid) and property_exists($node, 'coexp') and $node->coexp != 0) {
    return;
  } 

  // Remove white space around field values.
  $node->coexp_name = trim($node->coexp_name);
  $node->coexp_path = trim($node->coexp_path);
}  

/**
 * Implements hook_insert().
 * 
 * This function is called on insert of a new node
 * of type 'chado_coexp' and inserts the necessary information to 
 * public chado_coexp table. 
 * 
 * @ingroup tripal_coexp
 */
function chado_coexp_insert($node) {

  // the organism_id should unique
  // the name, path can be any content
  // output an error if organism exist
    $args = array(
      ':organism_id'=>$node->organism_id
    );
    $sql = "SELECT nid FROM chado_coexp
      WHERE organism_id=:organism_id";
    $status = db_query($sql, $args)->fetchField();

    if ($status) {
      form_set_error('organism_id', 'Organism should be unique.');
      tripal_report_error('tripal_coexp', TRIPAL_WARNING,
        'Insert Co-Expression: record already exists: %organism_id',
        array(
          '%organism_id' => "Organism = $node->organism_id",
        ));
    }

  // Remove white space around field values.
  $node->coexp_name = trim($node->coexp_name);
  $node->coexp_path = trim($node->coexp_path);

  // Actually insert the record.
  // get a tripal job to insert database
  $record = new stdClass();
  $record->nid         = $node->nid;
  $record->name        = $node->coexp_name;
  $record->path        = $node->coexp_path;
  $record->organism_id = $node->organism_id;
  $status = drupal_write_record('chado_coexp', $record);
 
  // add expression value to chado.feature_expression using tripal_job
  if ($node->coexp_load == 1) {
    tripal_coexp_load_job($node);
  }
} 

/**
 * Implements hook_load().
 * 
 *  When a node is requested by the user this function is called to allo us 
 *  to add auxiliary data to the node object.  
 * 
 * @ingroup tripal_coexp
 */
function chado_coexp_load($nodes) {
  foreach ($nodes as $nid => $node) {
    switch ($node->type) {
      case 'chado_coexp':
        $r = db_query("SELECT * FROM chado_coexp WHERE nid=:nid", array(':nid'=>$nid))->fetchObject();
        $node->coexp = $r;
        $node->title = $r->name;
    }
  } 
} 

/**
 * Implements hook_node_insert(). 
 * it only update node title and url 
 * 
 * @ingroup tripal_coexp
 */ 
function tripal_coexp_node_insert($node) {
  switch ($node->type) {
    case 'chado_coexp':
      // dpm($node, 'tripal node insert');
      if (property_exists($node, 'coexp')) {
        $node->title = $node->coexp->name;
      } else {
        $node->title = $node->coexp_name;
      } 
      break;
  }
}

/**
 * Implements hook_node_update().
 * This node update is different with content type update.
 * it only update node title
 *
 * @ingroup tripal_coexp
 */
function tripal_coexp_node_update($node) {
  switch ($node->type) {
    case 'chado_coexp':
      if (property_exists($node, 'coexp')) {
        $node->title = $node->coexp->name;
      } else {
        $node->title = $node->coexp_name;
      }
      break; 
  }
} 

/**
 * Implements hook_update().
 * 
 * @ingroup tripal_coexp
 */
function chado_coexp_update($node) {

  // Remove white space around field values.
  $node->coexp_name = trim($node->coexp_name);
  $node->coexp_path = trim($node->coexp_path);

  $values = array(
    'name'        => $node->coexp_name,
    'path'        => $node->coexp_path,
    'organism_id' => $node->organism_id,
  );

  $status = db_update('chado_coexp')->fields($values)->condition('nid', $node->nid)->execute();

  if ($node->coexp_load == 1) {
    tripal_coexp_load_job($node);
  }

  if (!$status) {
    drupal_set_message(t('Unable to update Co-Expression File.'), 'warning');
    tripal_report_error('tripal_coexp', TRIPAL_WARNING, 'Unable to update Co-Expression File where values: %values', array('%values' => print_r($values, TRUE)));
  } 
} 

/**
 * Implements hook_delete().
 * 
 * Delete coexp from both drupal and chado databases. Check dependencies before
 * deleting from chado. 
 * 
 * @ingroup tripal_coexp
 */
function chado_coexp_delete($node) {

  // Remove data from drupal tables including chado_coexp, node, and node_revisions
  $sql_del = "DELETE FROM chado_coexp WHERE nid = :nid";
  db_query($sql_del, array(':nid' => $node->nid));
  $sql_del = "DELETE FROM node WHERE nid = :nid AND vid = :vid";
  db_query($sql_del, array(':nid' => $node->nid, ':vid' => $node->vid));
  $sql_del = "DELETE FROM node_revision WHERE nid = :nid AND vid = :vid";
  db_query($sql_del, array(':nid' => $node->nid, ':vid' => $node->vid));

  $message = "Warning: the Co-Expression File ". $node->title ." has been removed from chado_coexp table.
  But the Expression value still in chado.feature_relationship which needs manually removed.
  ";
  drupal_set_message($message, 'status');
} 

/**
 * Implements hook_node_view().
 *
 * @ingroup tripal_coexp
 */
function tripal_coexp_node_view($node, $view_mode, $langcode) {
  switch ($node->type) {
      case 'chado_feature':
      if ($view_mode == 'full') {
        $node->content['tripal_coexp'] = array(
          '#markup' => theme('tripal_feature_coexp_list', array('node' => $node)),
          '#tripal_toc_id' => 'coexp',
          '#tripal_toc_title' => 'Co-expression',
          '#weight' => -98,
        );
      }
      break;
  }
}
