<?php

/**
 * Batch 'finished' callback 
 * @param type $success
 * @param type $results
 * @param type $operations
 */
function _finished_1($success, $results, $operations) {
  if ($success) {
    // Here we could do something meaningful with the results.
    // We just display the number of nodes we processed...
    $message = count($results['pidmissingdoi']) . ' objects have missing DOI.<br/>';
    $_SESSION['pidsmissingdois'] = $results['pidmissingdoi'];
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = 'An error occurred while processing ' . $error_operation[0] . ' with arguments :' . print_r($error_operation[0], true);
  }
  drupal_set_message($message);
}

/**
 * Batch 'finished' callback 
 * @param type $success
 * @param type $results
 * @param type $operations
 */
function _finished_2($success, $results, $operations) {
  if ($success) {
    // Here we could do something meaningful with the results.
    // We just display the number of nodes we processed...
    $message = count($results['pidmissingdoi']) . ' objects have missing DOI.<br/>';
    $_SESSION['pidsmissingdois'] = $results['pidmissingdoi'];
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = 'An error occurred while processing ' . $error_operation[0]
        . ' with arguments :' . print_r($error_operation[0], true);
  }
  drupal_set_message($message);
}

/**
 * Multistep form
 * @param type $form_state
 * @return string
 */
function fba_repo_manager_multistep_form($form_state = NULL) {

  $prefix = islandora_doi_get_prefix();
  $hostname = islandora_doi_get_hostname();
  $username = islandora_doi_get_username();
  $haspassword = islandora_doi_has_password();

  if (empty($prefix) || empty($hostname) || empty($username) || (!$haspassword)) {
    $form['plaintext'] = array(
      '#type' => 'markup',
      '#value' => 'The Islandora DOI configuration values have not yet been '
      . 'fully completed.<br />',);
  }
  else {
    $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;

    $form['step_display'] = array(
      '#type' => 'item',
      '#value' => 'step ' . $step,
    );
    if ($step === 1) {
      $form['plaintext'] = array(
        '#type' => 'markup',
        '#value' => 'This step searches for published datasets with a missing '
        . 'DOI entry in the DTCA datastream<br />',);
    }
    if ($step === 2) {
      $nummissingdois = 0;
      if ($_SESSION['pidsmissingdois']) {
        foreach ($_SESSION['pidsmissingdois'] as $pidmissingdoi) {
          $form['doipid' . $nummissingdois] = array(
            '#type' => 'markup',
            '#value' => 'This object has missing doi: ' . $pidmissingdoi . '<br />',
          );
          $nummissingdois++;
        }
      }
      $form['doipidtotal'] = array(
        '#type' => 'markup',
        '#value' => 'Total: ' . ($nummissingdois) . '<br />',
      );
      if ($nummissingdois > 0) {
        $form['instructions'] = array(
          '#type' => 'markup',
          '#value' => 'Click Go to resubmit each PID for DOI minting.<br />',
        );
      }
    }
    if (($step === 1) || (($step === 2) && ($nummissingdois > 1))) {
      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Go',
      );
    }
  }
  return $form;
}

/**
 * hook submit
 * 
 * @param type $form
 * @param type $form_state
 */
function fba_repo_manager_multistep_form_submit($form, &$form_state) {
  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;
  switch ($step) {
    case 1:
      drupal_set_message('step 1 completed');
      batch_set(_batch_1());
      break;
    case 2:
      drupal_set_message('step 2 completed');
      batch_set(_batch_2());
      $form_state['redirect'] = 'repomgr/doi_3';
      break;
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
function _batch_1() {
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  $operations = array();

  $all_pids = _doi_get_pids_from_collections();
  //limit is set to 1 million as there's no "all" value for this function
  foreach ($all_pids as $pid) {
    $operations[] = array('_op_1', array($pid));
  }
  // Create an array which contains an array of the operations to
  // perform and a method to call when the operations are all finished
  $batch = array(
    'operations' => $operations,
    'finished' => '_finished_1',
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/repomgr.doi.inc',
    'title' => t('Process RDF for published objects that should have a DOI'),
    'init_message' => t('Initialising...'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('_batch_1 encountered an error.'),
  );
  return $batch;
}

/**
 * Batch operation for batch 1
 * @param type $memberpid
 * @param type $context
 */
function _op_1($memberpid, &$context) {
    module_load_include('inc', 'fedora_repository', 'api/tuque');

  // get the mads datastream for this object
  $fedora_object = islandora_object_load($memberpid);
  $dtca = $fedora_object['DTCA'];
  $dtcacontent = $dtca->content;
  $sxmldtca = new SimpleXMLElement($dtcacontent);
  $minteddoi = (string) $sxmldtca->doi;
  if (empty($minteddoi)) {
    //if $minteddoi is empty, then this needs flagging to repomgr
    $context['results']['pidmissingdoi'][] = $memberpid;
  }

  // Optional message displayed under the progressbar.
  $context['message'] = t('Working on @pid', array('@pid' => $memberpid));
}

/**
 * 
 * @return array
 */
function _batch_2() {
  if ($_SESSION['pidsmissingdois']) {
    $pidsmissingdois = $_SESSION['pidsmissingdois'];
  }
  $operations = array();
  foreach ($pidsmissingdois as $pidmissingdoi) {
    $operations[] = array('_op_2', array($pidmissingdoi));
  }

  $batch = array(
    'operations' => $operations,
    'finished' => '_finished_2',
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/repomgr.doi.inc',
    // We can define custom messages instead of the default ones.
    'title' => t('Processing batch 2'),
    'init_message' => t('Batch 2 is starting.'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('Batch 2 has encountered an error.'),
  );
  return $batch;
}

/**
 * Batch operation for batch 2 
 * 
 * @param type $pid
 * @param type $context
 */
function _op_2($pid, &$context) {
  module_load_include('module', 'fba_doi', 'fba_doi');

  $hasdoi = _alreadyhasdoi($pid);

  if ($hasdoi === false) {
    $minteddoi = mintdoi($pid);
    if (empty($minteddoi)) {
      //if $minteddoi is empty, then this needs flagging to repomgr
      $context['results']['pidmissingdoi'][] = $pid;
    }
  }
  // Optional message displayed under the progressbar.
  $context['message'] = t('Working on @pid', array('@pid' => $pid));
}

/**
 *  Populate an array of all the PIDS across the 4 geovocab collections
 * @return type
 */
function _doi_get_pids_from_collections() {
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');

  $query = <<<EOD
  select \$object from <#ri>
    where (\$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/dtcavon:collection> or
           \$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/dtcaeden:collection> or
           \$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/dtcawensum:collection> or
           \$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/fbads:collection>)
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
 * 
 * @param type $pid
 * @return type
 */
function _alreadyhasdoi($pid) {
  module_load_include('inc', 'fedora_repository', 'api/tuque');
  module_load_include('module', 'islandora_doi', 'islandora_doi');
  module_load_include('module', 'fba_doi', 'fba_doi');
  

  $fedora_object = islandora_object_load($pid);
  $dtca = $fedora_object['DTCA'];
  $dtcacontent = $dtca->content;

  $reformattedpid = str_replace(':', '', $pid);

  //Check and reassociate or newly mint the DOI
  $checkeddoi = fba_check_doi($reformattedpid);
  if ($checkeddoi === false) {
    //check has failed for some reason (check watchdog logs)
  }
  else {
    //3. Add the doi to /dtca:Dataset/dtca:doi
    $sxmldtca = new SimpleXMLElement($dtcacontent);
    $sxmldtca->doi = $checkeddoi;

    $fedora_object['DTCA']->content = $sxmldtca->asXML();
  }
  return $checkeddoi;
}
