<?php

/*
 * Export form for TourML Tour/TourSets
 */
function tap_tourml_export_form($form, &$form_state) {
	$form['entity_id'] = array(
		'#title' => t('TourSet/Tour'),
		'#type' => 'textfield',
		'#description' => t('Specify the TourSet/Tour to export'),
		'#required' => TRUE,
		'#default_value' => isset($form_state['build_info']['args'][0]) ? $form_state['build_info']['args'][0] : '',
		'#autocomplete_path' => 'admin/content/tourml-export/autocomplete',
      	'#element_validate' => array('tap_export_entity_autocomplete_validate'),
      	'#value_callback' => 'tap_export_entity_autocomplete_value',  	
	);
	
	$default_bundle_name = '';
	if(isset($form_state['build_info']['args'][0]) && is_numeric($form_state['build_info']['args'][0])) {
		$node = node_load($form_state['build_info']['args'][0]);
		if($node) {
			$default_bundle_name = _clean_bundle_id($node->title);
		}
	}
	
	$form['bundle_name'] = array(
		'#title' => t('Bundle Name'),
		'#type' => 'textfield',
		'#description' => t('Specify the bundle name.'),
		'#required' => TRUE,
		'#default_value' => $default_bundle_name,
		'#element_validate' => array('tap_export_bundle_id_validate'),
	);
	
    $form['export_options'] = array(
		'#title' => t('Export Options'),
		'#type' => 'fieldset',
    );

	$form['export_options']['bundle_type'] = array(
		'#title' => t('Bundle Type'),
		'#type' => 'select',
		'#description' => t('Select the bundle type to use for exporting.'),
		'#options' => array('zip' => 'Zip', 'ios' => 'iOS'),
		'#default_value' => 'zip',
		'#ajax' => array(
			'callback' => 'tap_export_separate_languages_callback',
			'wrapper' => 'separate-languages-option',
		),
	);
	
	$languages = array();
	foreach(language_list() as $key => $item) {
		$languages[$key] = $item->native;
	}
	
	$form['export_options']['languages'] = array(
		'#title' => t('Languages'),
		'#type' => 'select',
		'#description' => t('Select the language(s) to include in the bundle.'),
		'#options' => $languages,
		'#multiple' => TRUE,
	);
	
	if(empty($form_state['values']['bundle_type']) || 
		(isset($form_state['values']['bundle_type']) && $form_state['values']['bundle_type'] != 'ios')) {
		$form['export_options']['separate_languages'] = array(
			'#title' => t('Separate Languages'),
			'#type' => 'select',
			'#description' => t('Specify whether languages should be seperated in different tourML files.'),
			'#options' => array('No', 'Yes'),
			'#default_value' => 0,
			'#prefix' => '<div id="separate-languages-option">',
			'#suffix' => '</div>',
		);
	} else {
		$form['export_options']['separate_languages'] = array(
			'#type' => 'hidden',
			'#value' => 0,
			'#prefix' => '<div id="separate-languages-option">',
			'#suffix' => '</div>',			
		);
	}
	
	$form['submit'] = array(
		'#value' => t('Export'),
		'#type' => 'submit',
	);
	
	return $form;
}

/*
 * Ajax callback for bundle type selection
 */
function tap_export_separate_languages_callback($form, $form_state) {
	return $form['export_options']['separate_languages'];
}

/*
 * Autocomplete function for collection set node references
 */
function tap_export_entity_autocomplete($string) {
	$query = db_select('node', 'n')
		->fields('n', array('nid', 'title'))
		->condition('n.title', '%' . $string . '%', 'LIKE')
		->condition('n.type', 'tour', '=')
		->range(0, 10);
	$results = $query->execute();
	
	$matches = array();
    foreach($results as $row) {
		$matches[$row->title . " [nid:$row->nid]"] = '<div class="reference-autocomplete">' . $row->title . '</div>';
    }
    return drupal_json_output($matches);	
}

/*
 * Build display for autocompleter
 */
function tap_export_entity_autocomplete_value($element, $input = FALSE, $form_state) {
	if($input === FALSE) {
		$nid = $element['#default_value'];
		if(!empty($nid) && is_numeric($nid)) {
			$node = node_load($nid);
			if($node && $node->type == 'tour') {
				$value = $node->title;
				$value .= ' [nid:' . $nid . ']';
				return $value;
			} else {
				return '';
			}
		}
	}
}

/*
 * Validate autocompleter
 */
function tap_export_entity_autocomplete_validate($element, &$form_state, $form) {
	if (!isset($element['#field_parents'])) {
    	$element['#field_parents'] = array();
  	}
	if(!isset($element['#field_name'])) {
		$element['#field_name'] = $element['#name'];
	}
	if(!isset($element['#language'])) {
		$element['#language'] = 'und';
	}
	
	$field = field_widget_field($element, $form_state);
	$instance = field_widget_instance($element, $form_state);

	$value = $element['#value'];
	$nid = NULL;
	if (!empty($value)) {
    	preg_match('/^(?:\s*|(.*) )?\[\s*nid\s*:\s*(\d+)\s*\]$/', $value, $matches);
    	if (!empty($matches)) {
      		list(, $title, $nid) = $matches;
    	}
	}
	form_set_value($element, $nid, $form_state);
}

/*
 * Validate bundle id
 */
function tap_export_bundle_id_validate($element, &$form_state, $form) {
	$cleaned_bundle_id = _clean_bundle_id($form_state['values']['bundle_name']);
	if ($form_state['values']['bundle_name'] != $cleaned_bundle_id) {
		form_set_error('bundle_name', t('Invalid Bundle Name. Only alphanumeric characters and underscores are allowed.'));
	}
}

/*
 * Submit function for TourML Export
 * Todo: refactor to handle bundling methods other than zip
 */
function tap_tourml_export_form_submit(&$form, &$form_state) {
	global $language;

	// retrieve list of available languages
	$language_list = language_list();
	$node = node_load($form_state['values']['entity_id']);
	$bundle_type = $form_state['values']['bundle_type'];
	$bundle_name = $form_state['values']['bundle_name'];
	$seperate_bundle = (bool)$form_state['values']['separate_languages'];
	
    // Set tourml settings
    $settings = array(
        'return_object' => TRUE,
        'absolute'      => FALSE,
        'base_path'		=> 'assets',
    );

	// set default language if none were provided
	if(empty($form_state['values']['languages'])) {
		$form_state['values']['languages'][] = 'en';
	}

	$title = _clean_file_text($node->title);

	// Check for presense of zip extension
    if (!class_exists('ZipArchive')) {
        drupal_set_message("You must install the php zip extension to export bundles.", 'error');
        return;
    }

    // Delete old bundle from node
    if (isset($node->field_tour_bundle[$node->language][0]['fid'])) {
        $file = file_load($node->field_tour_bundle[$node->language][0]['fid']);
        $file->status = 0;
        file_field_delete_file($node->field_tour_bundle[$node->language][0], 'field_tour_bundle', 'node', $node->nid);
        unset($node->field_tour_bundle[$node->language][0]);
    }

    $path = file_directory_temp() . "/$title-bundle.zip";

    // If we have a stale tmp file delete it
    if (file_exists($path)) {
        unlink($path);
    }

    $bundle = _zip_init($path, ZIPARCHIVE::CREATE);
	
	$bdir = "$title.bundle";
	$bundle->addEmptyDir($bdir);
	// generate plist file
	tap_export_bundle_plist_file($bundle, $bdir, $bundle_name);

	// iterate over languages
	foreach($form_state['values']['languages'] as $lang) {
		// set current language
		$language = $language_list[$lang];
		
		// setup language directory for ios
		$lang_dir = $bdir . '/' . $lang . '.lproj';
		$asset_dir = $lang_dir . '/assets';
		$bundle->addEmptyDir($lang_dir);
		$bundle->addEmptyDir($asset_dir);
		
	    $tourml         = tap_tourml_render($node, $settings);
	    $operations[]   = array('tap_export_bundle_string', array($path, $lang_dir, $tourml->tourml));
		
	    // Add file assets
	    foreach($tourml->assets as $asset) {
	        if (isset($asset['fid'])) {
	            $operations[] = array('tap_export_bundle_asset', array($path, $asset_dir, $asset['uri']));
	        }
	    }
	}

    $operations[]   = array('tap_export_bundle_tour_save', array($node, $path));

    $batch = array(
        'title'         => t('TourML Bundle Export'),
        'operations'    => $operations,
        'finished'      => 'tap_export_bundle_finish',
        'file'          => drupal_get_path('module', 'tap') . '/tap.export.inc',
    );

    batch_set($batch);
}

/**
 *
 * Export a downloadable tourml file
 * @param unknown_type $node
 */
function tap_export_tourml($node, $download = TRUE) {
	$title = _clean_file_text($node->title);

	$tourml = tap_tourml_render($node);

    header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
    header('Content-Type: text/xml');

    if ($download) {
        header('Content-Disposition: attachment; filename="' . $title . '.xml"');
    }

    die(print $tourml);
}

/**
 * Bundle a string into a file inside of a zip archive
 *
 * @param $path
 * Path to the zip archive
 * @param $dir
 * The directory to save the asset in
 * @param $string
 * The string to insert into the file
 */
function tap_export_bundle_string($path, $dir, $string, &$context) {
    $bundle = _zip_init($path);
    $bundle->addFromString("$dir/tour.xml" , $string);
}

/**
 * Bundle a string into a file inside of a zip archive
 *
 * @param $path
 * Path to the zip archive
 * @param $dir
 * The directory to save the asset in
 * @param $uri
 * the uri of the file to store in the zip file
 */
function tap_export_bundle_asset($path, $dir, $uri, &$context) {
    $bundle = _zip_init($path);
    list($schema, $file_path) = explode('://', $uri);
    $real_path = drupal_realpath($uri);
    $bundle->addFile($real_path, $dir . '/' . $file_path);

    $context['results'][] = $file_path;
}

function tap_export_bundle_tour_save($node, $file_path, &$context) {
    global $user;
    $settings       = field_info_instance('node', 'field_tour_bundle', 'tour');
    $dest           = 'public://' . $settings['settings']['file_directory'] . '/' . basename($file_path);
    $trans          = db_transaction();

    try {
        // Begin building file object.
        $file           = new stdClass();
        $file->uid      = $user->uid;
        $file->status   = 0;
        $file->filename = trim(basename($file_path, '.'));
        $file->uri      = $file_path;
        $file->filemime = file_get_mimetype($file->filename);
        $file->filesize = filesize($file_path);

        if (rename($file_path, drupal_realpath($dest))) {
            $file->uri      = $dest;

            drupal_chmod($file->uri);
            if ($file = file_save($file)) {
                $node->field_tour_bundle[$node->language][0] = (array)$file;
                $node->field_tour_bundle[$node->language][0]['display'] = 1;
                node_save($node);
				
				$context['results']['bundle'] = file_create_url($dest);
            }
        } else {
            throw new Exception('Unable to move uploaded file');
        }
    }
    catch(Exception $e) {
        $message = 'Error creating bundle: ' . basename($file_path);
        watchdog('tap', $message, array(), WATCHDOG_ERROR);
        drupal_set_message($message, 'error');
        $trans->rollback();
    }
}

function tap_export_bundle_finish($success, $results, $operations) {
    if ($success && isset($results['bundle'])) {
        $message = 'Bundle has been created with ' . format_plural(count($results), 'One file included.', '@count files included.');
		$message .= ' ' . l('Download bundle', $results['bundle']);
    } else {
        $message = t('An error occurred');
    }

    drupal_set_message($message);
}

/*
 * Generate a plist file
 */
function tap_export_bundle_plist_file($bundle, $dir, $bundle_name) {
	global $base_url;

	$url = explode('.', str_replace('http://', '', $base_url));
	$url = array_reverse($url);
	$url = implode('.', $url);
	
	$bundle_id = $url . '.' . $bundle_name;
	
	$plist_file = theme('ios_plist_xml', array('bundle_name' => $bundle_name, 'bundle_id' => $bundle_id));
	$bundle->addFromString("$dir/Info.plist" , $plist_file);
}

function _clean_bundle_id($text) {
	return preg_replace('/[^A-Za-z0-9_]/', '', $text);
}