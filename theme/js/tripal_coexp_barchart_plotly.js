
rnaseq_histogram(rnaseq_exps,experiments);

/**
 * plot histogram according to input factors
 */
function rnaseq_histogram_plot(div, title_name, type_name, factor1, factor2, experiments, values, sd) {

 	// the value and sd are array which include two values for two co-expressed genes
	var group = new Object();

		x1_name = new Array();
		y1_value = new Array();
		y1_sd = new Array();

		x2_name = new Array();
		y2_value = new Array();
		y2_sd = new Array();

		for (var exp_id in values) {
			var exp_name = experiments[exp_id].name;
            var value = values[exp_id];
            var sd_value = 0;
            if ( typeof sd[exp_id] != 'undefined') {
                sd_value = sd[exp_id];
				y1_sd.push(sd_value[0]);
				y2_sd.push(sd_value[1]);
            }
            link = '<a href="/experiment/' + exp_id + '">' + exp_name + '</a>';
            x1_name.push(link);
            y1_value.push(value[0]);

			x2_name.push(link);
			y2_value.push(value[1]);
        }

		if (x1_name.length > 50) {
			var trace1 = { 
				y: x1_name, 
				x: y1_value, 
				name: coexp_data[0],
				orientation: 'h',
				error_x: {
              	  type: 'data',
              	  array: y1_sd,
              	  visible: true
            	},
            	type: 'bar'
			};

			var trace2 = {
                y: x2_name,
                x: y2_value,
                name: coexp_data[1],
                orientation: 'h',
                error_x: {
                  type: 'data',
                  array: y2_sd,
                  visible: true
                },
				xaxis: 'y2',
 				yaxis: 'x2',
                type: 'bar'
            };

			var trace_data = [trace1, trace2];

			var layout = {
				autosize: false,
				width: 650,
				height: 1800,
				margin: {l: 50, r: 50, b: 100, t: 100, pad: 4 },
            	title: title_name,
            	//xaxis: { title: type_name },
            	//barmode: 'group',
				grid: {rows: 1, columns: 2, pattern: 'independent'},
				/*
				xaxis: {
    				domain: [0, 0.45],
    				anchor: 'y1'
  				},
  				yaxis: {
    				domain: [0.5, 1],
    				anchor: 'x1'
  				},
  				xaxis2: {
    				domain: [0.55, 1],
    				anchor: 'y2'
  				},
  				yaxis2: {
    				domain: [0.8, 1],
    				anchor: 'x2'
  				}, */
			};
        	Plotly.newPlot(div, trace_data, layout);
		}

}

/**
 * plot histogram
 */
function rnaseq_histogram(rnaseq_exps, experiments) {

	// order of default value type
	var value_types = new Array('RPKM', 'FPKM', 'RPM', 'raw_count');

			for (var pid in rnaseq_exps) {
				var title_name = '<a href="/bioproject/' + pid + '">' + rnaseq_exps[pid].name + '</a>';
				var desc = rnaseq_exps[pid].desc;
				var desc_html = '<p><b>Description: </b> <br>';
				for(var d in desc) {
					desc_html += desc[d] + "<br>";
				}
				desc_html += '</p>';

				var type_name;
				var sd_name;
				var values;
				var sd = new Array;
				for (var type in value_types) {
					type_name = value_types[type];
					sd_name = 'SD_' + type_name;

					if (typeof rnaseq_exps[pid][type_name] !== 'undefined') {
						values = rnaseq_exps[pid][type_name];
						if (typeof rnaseq_exps[pid][sd_name] !== 'undefined') {
							sd = rnaseq_exps[pid][sd_name];
						}
						break;
					}					
				}

				var factor1 = undefined;
				var factor2 = undefined;

				var subdiv = document.createElement("div");
				subdiv.id = 'tripal-rnaseq-histogram-' + pid;
				jQuery('#tripal-rnaseq-histogram').append(subdiv); 
				rnaseq_histogram_plot(subdiv.id, title_name, type_name, factor1, factor2, experiments, values, sd);

				// create div for description 
				var subdiv2 = document.createElement("div");
				subdiv2.id = 'tripal-rnaseq-desc-' + pid;
				jQuery('#tripal-rnaseq-histogram').append(subdiv2);
				jQuery('#' + subdiv2.id).html(desc_html);
			}
			jQuery('#tripal-rnaseq-description').html('');
}

