<?php

/**
 * Multistep form
 * @param type $form_state
 * @return string
 * 
 */
function fba_repo_manager_multistep_repair_form($form_state = NULL) {
  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;
  switch ($step) {
    case 1: {
        $form['plaintext'] = array(
          '#type' => 'markup',
          '#value' => 'Select ALL (for all dataset pids) or the individual PIDs separated by | no spaces <br />',
        );
        $form['pidlist'] = array(
          '#type' => 'textfield',
          '#default_value' => 'ALL',
          '#description' => 'Valid values are "ALL" or a pidlist separated by |, no spaces',
        );
        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => 'Go',
        );
        break;
      }
    case 2: {
        if (is_null($_SESSION['batch1_markup'])) {
          //No validation errors, so we can allow the next cleanup phase
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'Validation passed successfully, click button to initiate repair batch phase. <br/>',
          );
          /*
          $form['submit'] = array(
            '#type' => 'submit',
            '#value' => 'Go',
          );*/
        }
        else {
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => $_SESSION['batch1_markup'],
          );
        }
        break;
      }
    case 3: {
        $form['plaintext'] = array(
          '#type' => 'markup',
          '#value' => 'Repairs passed successfully. <br/>',
        );

        $form['plaintext'] = array(
          '#type' => 'markup',
          '#value' => $_SESSION['batch2_markup'],
        );

        break;
      }
  }
  return $form;
}

function get_all_datacomponents($pid) {
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');

  $object = islandora_object_load($pid);

  if ($object) {
    $predicates = sidora_return_fba_resource_rdfs();
    foreach ($predicates as $predicate) {
      $rels = $object->relationships->get('info:fedora/fedora-system:def/relations-external#', $predicate);
      if (count($rels) > 0) {
        foreach ($rels as $rel) {
          $relsresults[] = $rel['object']['value'];
        }
      }
    }
  }
  return $relsresults;
}

/**
 * hook submit
 * 
 * @param type $form
 * @param type $form_state
 */
function fba_repo_manager_multistep_repair_form_submit($form, &$form_state) {
  module_load_include('module', 'tuque_wrapper ', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/fba');

  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;
  switch ($step) {
    case 1: {
        $pidlist = $form_state['values']['pidlist'];
        batch_set(_repair_batch_1($pidlist));
        break;
      }
    case 2: {
        batch_set(_repair_batch_2());
        break;
      }
  }
  $form_state['storage']['step'] = $step + 1;
}

/** Start of Batch 1 functions * */

/**
 * 
 * @return array
 */
function _repair_batch_1($pidlist) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');

  $operations = array();

  if (strcmp($pidlist, 'ALL') === 0) {
    $sparqlQuery = 'SELECT ?pid WHERE { $pid <fedora-model:state> <fedora-model:Active>; <fedora-model:hasModel> ?model;
            FILTER (
            regex(str(?pid), "^info:fedora/dtcds") && 
            regex(str(?model), "^info:fedora/fba:dataset_cmodel$", "i"))}';
    $sparqlresult = ObjectHelper::performRiQuery($sparqlQuery, 'sparql');
    foreach ($sparqlresult as $notused => $pidvaluetext) {
      $pidarray[] = $pidvaluetext['pid'];
    }
  }
  else {
    $pidarray = explode('|', $pidlist);
  }
  //validation first
  foreach ($pidarray as $userdatasetpid) {
    $operations[] = array('_repair_validation_check', array($userdatasetpid));
  }

  // Create an array which contains an array of the operations to
  // perform and a method to call when the operations are all finished
  $batch = array(
    'operations' => $operations,
    'finished' => '_repair_finished_1',
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/repomgr.repair.inc',
    'title' => t('Validating terms in all datacomponents of private datasets'),
    'init_message' => t('Initialising...'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('_repair_batch_1 encountered an error.'),
  );
  $_SESSION['pidarray'] = $pidarray;
  return $batch;
}

/**
 * Batch operation function for batch 1
 *
 * @param type $pid
 * @param type $context
 */
function _repair_validation_check($pid, &$context) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/fbasidora/common');

  //Get the datacomponents of this object
  $results = get_all_datacomponents($pid);
  if ($results) {
    foreach ($results as $resultpid) {
      $result_object = islandora_object_load($resultpid);
      $namespaces = sidora_return_fba_csv_resources_namespaces();
      $respidns = substr($resultpid, 0, strpos($resultpid, ':'));
      if (in_array($respidns, $namespaces)) {
        //Get the csv file(s) from the OBJ* datastream(s) if one of the 5 csv datacomponent types
        foreach ($result_object as $datastream) {
          if (preg_match('/^OBJ/', $datastream->id)) {
            $objds = $result_object[$datastream->id];
            $filecontent = $objds->content;
            $firstlinebreak = strpos($filecontent, PHP_EOL);
            $secondlinebreak = strpos($filecontent, PHP_EOL, $firstlinebreak + strlen(PHP_EOL));
            $thirdlinebreak = strpos($filecontent, PHP_EOL, $secondlinebreak + strlen(PHP_EOL));
            $line1 = substr($filecontent, 0, $firstlinebreak);
            $line2 = substr($filecontent, $firstlinebreak + 1, $secondlinebreak - ($firstlinebreak + 1));
            $line3 = substr($filecontent, $secondlinebreak + 1, $thirdlinebreak - ($secondlinebreak + 1));
            //$fullcsv = array_map('str_getcsv', str_getcsv($filecontent, "\n"));
            //$vocabterms = $fullcsv[1];
            $vocabterms = explode(',', $line2);

            foreach ($vocabterms as $vocabterm) {
              $checked_result = (string) _check_vocab_term($vocabterm);
              if (is_null($checked_result)) {
                $invalid_vocab_terms[$resultpid][$key] = $vocabterm;
              }
            }

            //$unit_terms = $fullcsv[2];
            $unit_terms = explode(',', $line3);

            //Only one unit is allowed
            foreach ($unit_terms as $key => $unitterm) {
              $termarray = explode('~~', $unitterm);
              foreach ($termarray as $termarrayvalue) {
                $colonpos = strpos($termarrayvalue, ':');
                $termprefix = trim(substr($termarrayvalue, 0, $colonpos));
                switch ($termprefix) {
                  case 'Units': {
                      $termvalue = trim(substr($termarrayvalue, $colonpos + 1));
                      //N/A is a valid unit
                      if (strcmp($termvalue, 'N/A') === 0) {
                        $checked_unit_result = 'N/A';
                      }
                      else {
                        $checked_unit_result = _check_vocab_term(&$termvalue);
                      }
                      if ($checked_unit_result === NULL) {
                        $invalid_unit_terms[$resultpid][$key] = $termvalue;
                      }
                      else {
                        $valid_unit_terms[$resultpid][$key] = $termvalue;
                      }
                      $valid_CSV[$fileindex] = !($invalid_unit_terms[$resultpid]);

                      break;
                    }
                  case 'Taxon': {
                      $taxonvalue = trim(substr($termarrayvalue, $colonpos + 1));
                      $checked_taxon_result = _check_taxon_term(&$taxonvalue);
                      if ($checked_taxon_result === NULL) {
                        //there can be more than one in here
                        $invalid_taxon_terms[$resultpid][$key][] = $taxonvalue;
                      }
                      else {
                        $valid_taxon_terms[$resultpid][$key] = $taxonvalue;
                      }
                      break;
                    }
                  default: {
                      break;
                    }
                }
              }
            }
          }
        }
      }
    }
  }
  else {
    //$context['results']['error'][$pid] = 'Error - No Datacomponents held within ' . $pid;
  }

  //Validate 
  if (!is_null($invalid_unit_terms)) {
    foreach ($invalid_unit_terms as $key => $term) {
      $context['results']['invalid_unit_terms'][$key] = $term;
    }
  }

  if (!is_null($invalid_vocab_terms)) {
    foreach ($invalid_vocab_terms as $key => $term) {
      $context['results']['invalid_vocab_terms'][$key] = $term;
    }
  }

  if (!is_null($invalid_taxon_terms)) {
    foreach ($invalid_taxon_terms as $key => $term) {
      $context['results']['invalid_taxon_terms'][$key] = $term;
    }
  }

  // Optional message displayed under the progressbar.
  $context['message'] = t('Working on @pid', array('@pid' => $pid));
  $context['finished'] = 1;
}

/**
 * Batch 'finished' callback 
 * @param type $success
 * @param type $results
 * @param type $operations
 */
function _repair_finished_1($success, $results, $operations) {
  if ($success) {
    foreach ($results as $termtype => $result) {
      $batch1_markup .= $termtype . '<br/>';
      foreach ($result as $reskey => $resvalue) {
        $batch1_markup .= '  :  ' . $reskey . '<br/>';
        if (is_array($resvalue)) {
          foreach ($resvalue as $index => $termstring) {
            $intval = intval($index) + 1;
            $batch1_markup .= '  :  :  Column ' . $intval . ' = ' . $termstring . '<br/>';
          }
        }
        else {
          $batch1_markup .= '  :  :   ' . $resvalue . '<br/>';
        }
      }
      $batch1_markup .= '<br/>';
    }
    $message = 'Batch phase 1 complete.';
    $_SESSION['batch1_markup'] = $batch1_markup;
  }
  else {
    // An error occurred.
    $message = 'An error occurred while processing phase 1.';
    $_SESSION['batch1_markup'] = 'An error occurred during batch processing<br/>';
  }
  drupal_set_message($message);
}

/** Start of Batch 2 functions * */

/**
 * 
 * @return array
 */
function _repair_batch_2() {
  $pidarray = $_SESSION['pidarray'];

  //validation first
  foreach ($pidarray as $pid) {
    $operations[] = array('_repair_op_2', array($pid));
  }

  // Create an array which contains an array of the operations to
  // perform and a method to call when the operations are all finished
  $batch = array(
    'operations' => $operations,
    'finished' => '_repair_finished_2',
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/repomgr.repair.inc',
    'title' => t('Processing datacomponents and repairing predicates'),
    'init_message' => t('Initialising...'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('_repair_batch_2 encountered an error.'),
  );
  return $batch;
}

/**
 * Batch operation function for batch 2
 *
 * @param type $pid
 * @param type $context
 */
function _repair_op_2($pid, &$context) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('inc', 'sidora', 'includes/fbasidora/common');

  //Get the datacomponents of this object
  $results = get_all_datacomponents($pid);
  if ($results) {
    foreach ($results as $resultpid) {
      $result_object = islandora_object_load($resultpid);
      $namespaces = sidora_return_fba_csv_resources_namespaces();
      $respidns = substr($resultpid, 0, strpos($resultpid, ':'));
      if (in_array($respidns, $namespaces)) {
        //Get the csv file(s) from the OBJ* datastream(s)
        foreach ($result_object as $datastream) {
          $dsid = $datastream->id;

          if (preg_match('/^COLS/', $dsid)) {
            //Remove this datastream and later recreate them all
            $result_object->purgeDatastream($datastream->id);
          }
        }
        $result_object->purgeDatastream('RELS-INT');
        create_datacomponent_datastreams($resultpid);
        $context['results'][] = $resultpid;
      }
    }
  }
  // Optional message displayed under the progressbar.
  $context['message'] = t('Working on @pid', array('@pid' => $pid));
}

/**
 * Batch 'finished' callback 
 * @param type $success
 * @param type $results
 * @param type $operations
 */
function _repair_finished_2($success, $results, $operations) {
  if ($success) {
    $batch2_markup = 'PIDs processed:<br/>';
    foreach ($results as $result) {
      $batch2_markup .= '  :  ' . $result . '<br/>';
    }
    $message = 'Batch phase 2 complete.';
    $_SESSION['batch2_markup'] = $batch2_markup;
  }
  else {
    // An error occurred.
    $message = 'An error occurred while processing phase 2.';
  }
  drupal_set_message($message);
}

/** End of Batch 2 functions * */
  