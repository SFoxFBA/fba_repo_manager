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
    $message = 'Success.<br/>';
    $res = $results['pidsmissingdoi'];
    $_SESSION['pidsmissingdoi'] = $res;
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = 'An error occurred while processing ' . $error_operation[0] .
        ' with arguments :' . print_r($error_operation[0], true);
  }
  drupal_set_message($message);
}

/**
 * Batch 'finished' callback 
 * 
 * @param type $success
 * @param type $results
 * @param type $operations
 * @return none
 */
function _finished_2($success, $results, $operations) {
  if ($success) {
    // Here we could do something meaningful with the results.
    $_SESSION['pidsmissingdoi'] = $results['pidsmissingdoi']; //needed?
    $_SESSION['mintingfailed'] = $results['mintingfailed']; //needed?
    $_SESSION['registerfailed'] = $results['registerfailed']; //needed?
    $_SESSION['errormessage'] = $results['errormessage']; //needed?
    $_SESSION['pidsprocessed'] = $results['pidsprocessed']; //needed?
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = 'An error occurred while processing '
        . $error_operation[0]
        . ' with arguments :'
        . print_r($error_operation[0], true);
  }
  drupal_set_message($message);
}

/**
 * Multistep form
 * @param type $form_state
 * @return string
 * 
 */
function fba_repo_manager_multistep_form($form_state = NULL) {
  $fba_doi = new fba_doi();

  if ($fba_doi->fba_check_doi_configuration()) {
    $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;

    $form['step_display'] = array(
      '#type' => 'item',
      '#value' => 'step ' . $step,
    );
    switch ($step) {
      case 1: { //THIS GETS CALLED TWICE! WHY???
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'This step searches for published datasets with a missing '
            . 'DOI entry in the DTCA datastream<br />',);
          break;
        }
      case 2: {
          $rownumber = 0;
          if ($_SESSION['pidsmissingdoi']) {
            foreach ($_SESSION['pidsmissingdoi'] as $pidwithmissingdoi) {
              $form['row' . $rownumber] = array(
                '#type' => 'fieldset',
                '#collapsible' => FALSE,
                '#collapsed' => FALSE,
              );
              $form['row' . $rownumber]['dspid'] = array(
                '#type' => 'markup',
                '#value' => 'Dataset PID:' . $pidwithmissingdoi . '<br/>',
              );
              $form['row' . $rownumber]['checkbox'] = array(
                '#type' => 'checkbox',
                //'#value' => $objdsids[$i] . '-' . $objlabels[$i],
                '#title' => $pidwithmissingdoi,
                '#name' => 'objs' . $rownumber,
                '#return_value' => check_plain($pidwithmissingdoi),
                '#parents' => array('row' . $rownumber),
              );
              /*
                $form['doipid' . $nummissingdois] = array(
                '#type' => 'markup',
                '#value' => 'This object has missing doi: ' . $pidsmissingdoi . '<br />',
                ); */
              $rownumber++;
            }
          }
          $form['doipidtotal'] = array(
            '#type' => 'markup',
            '#value' => 'Total: ' . ($rownumber) . '<br />',
          );
          if ($rownumber > 0) {
            $form['instructions'] = array(
              '#type' => 'markup',
              '#value' => 'Click Go to resubmit selected PIDs for DOI minting.<br />',
            );
          }
          //unset($_SESSION['pidsmissingdoi']);
          break;
        }
      case 3: {
          //tried to mint $_SESSION[pidsmissingdoi], minting faile on $_SESSION[mintingfailed], errors were $_SESSION[erromessage]
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'Tried to mint these with a doi:<br/>',
          );
          if ($_SESSION['pidsprocessed']) {
            foreach ($_SESSION['pidsprocessed'] as $ppd => $ppdpid) {
              $form['plaintext'][] = array(
                '#type' => 'markup',
                '#value' => ' - ' . $ppd . '<br/>',
              );
            }
          }
          else {
            $form['plaintext'][] = array(
              '#type' => 'markup',
              '#value' => ' - none<br/>',
            );
          }

          $form['plaintext'][] = array(
            '#type' => 'markup',
            '#value' => 'These pids reported back with no doi:<br/>',
          );
          if ($_SESSION['pidsmissingdoi']) {
            foreach ($_SESSION['pidsmissingdoi'] as $pmd => $pmdpid) {
              $form['plaintext'][] = array(
                '#type' => 'markup',
                '#value' => ' - ' . $pmd . '<br/>',
              );
            }
          }
          else {
            $form['plaintext'][] = array(
              '#type' => 'markup',
              '#value' => ' - none<br/>',
            );
          }

          $form['plaintext'][] = array(
            '#type' => 'markup',
            '#value' => 'Metadata registration failed on these:<br/>',
          );
          if ($_SESSION['registerfailed']) {
            foreach ($_SESSION['registerfailed'] as $rf => $rfpid) {
              $form['plaintext'][] = array(
                '#type' => 'markup',
                '#value' => ' - ' . $rf . '<br/>',
              );
            }
          }
          else {
            $form['plaintext'][] = array(
              '#type' => 'markup',
              '#value' => ' - none<br/>',
            );
          }
          $form['plaintext'][] = array(
            '#type' => 'markup',
            '#value' => 'DOI Minting failed on these:<br/>',
          );
          if ($_SESSION['mintingfailed']) {
            foreach ($_SESSION['mintingfailed'] as $mf => $mfpid) {
              $form['plaintext'][] = array(
                '#type' => 'markup',
                '#value' => ' - ' . $mf . '<br/>',
              );
            }
          }
          else {
            $form['plaintext'][] = array(
              '#type' => 'markup',
              '#value' => ' - none<br/>',
            );
          }

          $form['plaintext'][] = array(
            '#type' => 'markup',
            '#value' => 'and we got these errors:<br/>',
          );
          if ($_SESSION['errormessage']) {
            foreach ($_SESSION['errormessage'] as $errorpid => $errorarray) {
              $form['plaintext'][] = array(
                '#type' => 'markup',
                '#value' => ' - ' . $errorpid . ' -> ' . $errorarray[0] . ' -> ' . $errorarray[1] . '<br/>',
              );
            }
          }
          else {
            $form['plaintext'][] = array(
              '#type' => 'markup',
              '#value' => ' - none<br/>',
            );
          }
          break;
        }
      default: {
          break;
        }
    }
    if (($step === 1) || (($step === 2) && ($rownumber > 0))) {
      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Go',
      );
    }
  }
  else {
    $form['plaintext'] = array(
      '#type' => 'markup',
      '#value' => 'The Islandora DOI configuration values have not yet been '
      . 'fully completed.<br />',);
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
    case 1: {
        drupal_set_message('step 1 completed');
        batch_set(_batch_1());
        break;
      }
    case 2: {
        drupal_set_message('step 2 completed');
        $post_data = $form_state['clicked_button']['#post'];
        foreach ($post_data as $key => $value) {
          if (preg_match('/^objs/', $key)) {
            $checkedboxespids[] = $value;
          }
        }
        batch_set(_batch_2($checkedboxespids));
        $form_state['redirect'] = 'repomgr/doi_3';
        break;
      }
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
 *
 * @param type $memberpid
 * @param type $context
 */
function _op_1($memberpid, &$context) {
  module_load_include('inc', 'fedora_repository', 'api/tuque');

  $fedora_object = islandora_object_load($memberpid);
  $dtca = $fedora_object['DTCA'];
  $dtcacontent = $dtca->content;
  $sxmldtca = new SimpleXMLElement($dtcacontent);
  $minteddoi = (string) $sxmldtca->doi;
  if (empty($minteddoi)) {
    //if $minteddoi is empty, then this needs flagging to repomgr
    $context['results']['pidsmissingdoi'][] = $memberpid;
  }

  // Optional message displayed under the progressbar.
  $context['message'] = t('Working on @pid', array('@pid' => $memberpid));
}

/**
 * 
 * @return array
 */
function _batch_2($checkedboxespids) {
  $operations = array();
  foreach ($checkedboxespids as $checkedboxespid) {
    $operations[] = array('_op_2', array($checkedboxespid));
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
  $context['results']['pidsprocessed'][$pid] = $pid;
  if ($hasdoi === false) {
    $fba_doi = new fba_doi();
    $metadatadoi = $fba_doi->registermetadata($pid);
    if ($metadatadoi !== FALSE) {
      $minteddoi = $fba_doi->mintdoi($pid);
      if (empty($minteddoi)) {
        //if $minteddoi is empty, then this needs flagging to repomgr
        $context['results']['pidsmissingdoi'][$pid] = $pid;
        $context['results']['mintingfailed'][$pid] = $pid;
        $context['results']['errormessage'][$pid] = $_SESSION['errormessage'];
      }
    }
    else {
      //registering failed so flag it
      $context['results']['registerfailed'][$pid] = $pid;
      $context['results']['errormessage'][$pid] = $_SESSION['errormessage'];
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
    where ((\$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/dtcavon:collection> or
            \$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/dtceden:collection> or
            \$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/dtcwensum:collection> or
            \$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/fbads:collection> or
            \$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/syngds:collection> or
            \$object  <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/fbads:collection>) and
            \$object  <info:fedora/fedora-system:def/model#hasModel> <info:fedora/fba:dataset_cmodel>)
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
  $fba_doi = new fba_doi();
  $checkeddoi = $fba_doi->fba_check_doi($reformattedpid);
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
