<?php

foreach ($rnaseq_exp as $project) {
	$pid = $project->project_id;
	$pname = $project->name;

	if (sizeof($project->RPKM) > 0) {
		$exp = $project->RPKM;
		$unit = 'RPKM';
	}
	elseif (sizeof($project->FPKM) > 0) {
		$exp = $project->FPKM;
		$unit = 'FPKM';
	}
	elseif (sizeof($project->RPM) > 0) {
		$exp = $project->RPM;
		$unit = 'RPM';
	}
	elseif (sizeof($project->raw_count) > 0) {
		$exp = $project->raw_count;
		$unit = 'raw_count';
	}

	//build table
	$headers = array('Sample', "$coexp_data[0] ($unit)", "$coexp_data[1] ($unit)");
    $rows = array();
	foreach ($exp as $sid => $e) {
		$sname = $samples[$sid]->name;
		$slink = l($sname, 'biosample/' . $sname, array('attributes' => array('target' => "_blank")));
		$rows[] = array($slink, $e[0], $e[1]);
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

    print '<div class="row"><div class="col-md-6 col-md-offset-3">';
    print theme_table($table);
    print '</div></div>';
}

