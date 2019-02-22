<?php
/**
 * @file
 * Functions used to install the module
 */

/**
 * Implements install_hook().
 *
 * Permforms actions when the module is first installed.
 *
 * @ingroup tripal_coexp
 */
//function tripal_coexp_install() {
  //tripal_coexp_add_cvterms();
  //use term associated_with (SO) to connect the co-express genes
//}

/**
 * add cvterms for feature relationship
 */
/**
function tripal_coexp_add_cvterms() {

  tripal_insert_cv(
    'tripal_coexp',
    'Contains property terms for tripal coexp.'
  );
    tripal_insert_cvterm(array(
      'name' => 'coexpression',
      'definition' => 'co-expression gene',
      'cv_name' => 'tripal_coexp',
      'db_name' => 'tripal',
    ));
}
*/

/** 
 * Implements hook_schema()
 * the coexp does not need schema. If you relation ship load, the old one will be repalced
 */
function tripal_coexp_schema() {
  // table for store co-expression file as node, which used for display in gene feature page 
  $schema['chado_coexp'] = array(
    'description' => t('The table for coexp node'),
    'fields' => array(
      'nid' => array(
        'description' => t('The primary identifier for a node.'),
        'type' => 'serial', 'unsigned' => true, 'not null' => true,
      ),
      'name' => array(
        'description' => t('The human-readable name for co-experession file.'),
        'type' => 'varchar', 'length' => 1023, 'not null' => true,
      ),
      'path' => array(
        'description' => t('The full path of the co-expression file.'),
        'type' => 'varchar', 'length' => 1023, 'not null' => true,
      ),
      'organism_id' => array(
        'description' => t('the organism id.'),
        'type' => 'int', 'size' => 'big', 'not null' => true,
      ),
    ),
    'indexes' => array(
      'name' => array('name'),
    ),
    'primary key' => array('nid'),
    'unique keys' => array(
      'nid' => array('nid'),
      'organism_id' => array('organism_id'),
    ),
  );

  return $schema;
} 

