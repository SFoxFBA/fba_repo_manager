<?php

/**
 * Hook form
 * 
 * @param type $form_state
 * @return string
 */
function fba_repo_manager_multistep_term_form($form_state) {
  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;

  if ($step === 1) {
    $form['step_display'] = array(
      '#type' => 'item',
      '#value' => 'Step ' . $step,
    );
    $form['plaintext'] = array(
      '#type' => 'markup',
      '#value' => 'This step searches for any term classed as orphan<br />',);
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Go',
    );
  }

  return $form;
}

/**
 * Hook form submit
 * 
 * @param type $form
 * @param reference $form_state
 */
function fba_repo_manager_multistep_term_form_submit($form, &$form_state) {
  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;

  switch ($step) {
    case 1: {
        batch_set(_term_batch_1());
        $form_state['redirect'] = 'repomgr/term_2';
        break;
      }
    /*
      case 2: {
      drupal_set_message('step 2 completed');
      batch_set(_term_batch_2());
      $form_state['redirect'] = 'repomgr/term_3';
      break; }

     */
  }

  $form_state['storage']['step'] = $step + 1;
}

/**
 * Batch 1
 *
 * This method does not yet do the work; it simply builds an array
 * of the work which needs to be done during the batch processing.
 * The results of this function will be passed to the batch processor
 * for actual processing.
 */
function _term_batch_1() {
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  $operations = array();

  $all_pids = _term_get_pids_from_collections();
  //limit is set to 1 million as there's no "all" value for this function
  foreach ($all_pids as $pid) {
    $operations[] = array('_term_op_1', array($pid));
  }
  // Create an array which contains an array of the operations to
  // perform and a method to call when the operations are all finished
  $batch = array(
    'operations' => $operations,
    'finished' => '_term_finished_1',
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/repomgr.term.inc',
    'title' => t('Process RDF to find objects defined as an orphan'),
    'init_message' => t('Initialising...'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('_term_batch_1 encountered an error.'),
  );
  return $batch;
}

/**
 * Batch operation for batch 1
 */
function _term_op_1($memberpid, &$context) {
  //module_load_include('inc', 'custom_search', 'includes/apachesolr_search');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');

  //check if this is the first one, and zero all the statistics
  if (!isset($context['results']['preferredterms'])) {
    $context['results']['preferredterms'] = 0;
    $context['results']['nonpreferredterms'] = 0;
    $context['results']['topterms'] = 0;
    $context['results']['toptermsarray'] = array();
    $context['results']['orphanterms'] = 0;
    $context['results']['orphantermsarray'] = array();
    $context['results']['hierrels'] = 0;
    $context['results']['equivrels'] = 0;
    $context['results']['assocrels'] = 0;
    $context['results']['georels'] = 0;
    $context['results']['geoterms'] = 0;
  }

  //some arrays for statistics
  $hierarchicalbt = array('hasBroaderAuthority');
  $hierarchicalnt = array('hasNarrowerAuthority');
  $equivalence = array('useFor', 'useInstead');
  $associative = array('hasRelatedAuthority');
  $geographic = array('hasGeographicFeature', 'isGeographicFeatureOf');
  //end of statistics arrays

  $rel_inverse["useInstead"] = "useFor";
  $rel_inverse["useFor"] = "useInstead";
  $rel_inverse["hasBroaderAuthority"] = "hasNarrowerAuthority";
  $rel_inverse["hasNarrowerAuthority"] = "hasBroaderAuthority";
  $rel_inverse["hasRelatedAuthority"] = "hasRelatedAuthority";
  $rel_inverse["hasGeographicFeature"] = "isGeographicFeatureOf";
  $rel_inverse["isGeographicFeatureOf"] = "hasGeographicFeature";

  // get the mads datastream for this object
  $item = new fedora_item($memberpid);

  $pidnamespace = substr($memberpid, 0, strpos($memberpid, ':'));
  if ((strcmp($pidnamespace, 'geopref') == 0) || (strcmp($pidnamespace, 'geononpref') == 0 )) {
    $context['results']['geoterms'] ++;
  }

  $relationships = array();
  $relsext = $item->get_datastream_dissemination('RELS-EXT');
  $mads = $item->get_datastream_dissemination('MADS');
  $simplemads = simplexml_load_string($mads);
  $madsvariant = $simplemads->variant->topic;
  $madsauthority = $simplemads->authority->topic;

  $authvar = '';
  if (!is_null($madsvariant)) {
    //it's a non preferred term
    $context['results']['nonpreferredterms'] ++;
    $authvar = (string) $madsvariant;
  }
  if (!is_null($madsauthority)) {
    //it's a preferred term
    $context['results']['preferredterms'] ++;
    $authvar = (string) $madsauthority;
  }

  // Parse the RELS-EXT into an associative array.
  $relsextxml = new DOMDocument();
  $relsextxml->loadXML($relsext);
  $relsextxml->normalizeDocument();

  $allTags = array();
  $allTags[] = $relsextxml->getElementsByTagNameNS('*', '*');

  foreach ($allTags as $tags) {
    foreach ($tags as $child) {
      $attns = $child->getAttributeNS('http://www.w3.org/1999/02/22-rdf-syntax-ns#', 'resource');
      if ($attns != NULL) {
        $value = preg_replace('/info:fedora\//', '', $attns);

        $relationships[$child->tagName][] = $value;
      }
    }
  }

  $countforbroaderterms = 0;
  $countfornarrowerterms = 0; //used to calculate "Top terms"/"Orphan terms"
  foreach ($relationships as $key => $pidarray) {
    foreach ($pidarray as $foreignpid) {
      //update the statistics
      if (in_array($key, $hierarchicalbt)) {
        $context['results']['hierrels'] ++;
        $countforbroaderterms++;
      }
      if (in_array($key, $hierarchicalnt)) {
        $context['results']['hierrels'] ++;
        $countfornarrowerterms++;
      }
      if (in_array($key, $equivalence)) {
        $context['results']['equivrels'] ++;
      }
      if (in_array($key, $associative)) {
        $context['results']['assocrels'] ++;
      }
      if (in_array($key, $geographic)) {
        $context['results']['georels'] ++;
      }
    }
  }
  if ($countforbroaderterms == 0) {
    if ($countfornarrowerterms == 0) {
      //can only be an orphan if it's a preferred term
      if (!is_null($madsauthority)) {
        //it's an orphan term
        $context['results']['orphanterms'] ++;
        $context['results']['orphantermsarray'][] = $authvar;
      }
    }
    else {
      //it's a top term
      $context['results']['topterms'] ++;
      $context['results']['toptermsarray'][] = $authvar;
    }
  }
  else {
    //it's a normal term, no stats required
  }
  // Optional message displayed under the progressbar.
  $context['message'] = t('Working on @pid', array('@pid' => $memberpid));
}

/**
 * Populate an array of all the PIDS across the 4 geovocab preferred term collections
 * @return array
 */
function _term_get_pids_from_collections() {
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');

  $query = <<<EOD
  select \$object from <#ri>
    where (\$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/fba:vocabpref_collection> or
           \$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/fba:geopref_collection> )
EOD;

  $query = htmlentities(urlencode($query));
  $content = '';
  $limit = 1000000;
  $offset = 0;
  $url = variable_get('fedora_repository_url', 'http://localhost:8080/fedora/risearch');
  $url .= "?type=tuples&flush=TRUE&format=csv&limit=$limit&offset=$offset&lang=itql&stream=on&query=" . $query;
  $content .= do_curl($url);
  $results = explode("\n", $content);
  $lines = preg_replace('/info:fedora\/|"object"/', '', $results);
  $allpids = array_values(array_filter($lines));
  return $allpids;
}

/**
 * Batch 'finished' callback 
 * 
 * @param type $success
 * @param type $results
 * @param type $operations
 */
function _term_finished_1($success, $results, $operations) {
  if ($success) {
    // Here we could do something meaningful with the results.
    // We just display the number of nodes we processed...

    $message = 'Number of preferred terms: ' . $results['preferredterms']
        . '<br/>';
    $message .= 'Number of nonpreferred terms: ' . $results['nonpreferredterms']
        . '<br/>';
    $message .= 'Number of top terms: ' . $results['topterms']
        . '<br/>';
    $message .= 'Number of hierarchical relations: ' . $results['hierrels']
        . '<br/>';
    $message .= 'Number of equivalent relations: ' . $results['equivrels']
        . '<br/>';
    $message .= 'Number of assoc relations: ' . $results['assocrels']
        . '<br/>';
    $message .= 'Number of geo relations: ' . $results['georels']
        . '<br/>';
    $message .= 'Number of geo terms: ' . $results['geoterms']
        . '<br/>';
    $message .= 'Number of orphan terms: ' . $results['orphanterms']
        . '<br/>';

    foreach ($results['orphantermsarray'] as $orphanterm) {
      $message .= ' - ' . $orphanterm . '<br />';
    }
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = 'An error occurred while processing ' . $error_operation[0] . ' with arguments :' . print_r($error_operation[0], true);
  }
  drupal_set_message($message);
}
