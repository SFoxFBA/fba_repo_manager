<?php

/**
 * Multistep form
 * @param type $form_state
 * @return string
 * 
 */
function fba_repo_manager_multistep_unpublish_form($form_state = NULL) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('module', 'fba_doi', 'fba_doi');

  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;
  // This is what tells the form which theme function to use
  /* $form['step_display'] = array(
    '#type' => 'item',
    '#value' => 'step ' . $step,
    ); */
  switch ($step) {
    case 1: { //THIS GETS CALLED TWICE...WHY!?!?
        unset($_SESSION['pidstounpublish']);
        unset($_SESSION['selectedpubpid']);
        unset($_SESSION['selectedpid']);
        unset($_SESSION['rdfremove']);
        unset($_SESSION['resetdtca']);
        unset($_SESSION['purgeobject']);

        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => 'Retrieve published activities',
        );

        break;
      }
    case 2: {

        //check that islandora/fba doi has been configured with values
        $fba_doi = new fba_doi();
        if ($fba_doi->fba_check_doi_configuration()) {
          $query_string = "SELECT ?pid ?pubpid WHERE { ?pid ?predicate ?object .
  ?pid <fedora-model:state> <fedora-model:Active> .
  ?pid <fedora-model:hasModel> <info:fedora/fba:activity_cmodel> .
  ?pid <http://fba.org.uk/schemas/fba/fba_rdf/1.0.0/hasPublishedVersion> ?pubpid}";

          //$rdfns = array_merge(sidora_return_fba_classes_namespaces(), sidora_return_fba_resources_namespaces());
          $query_results = ObjectHelper::performRiQuery($query_string, 'sparql');
          //Don't bother filtering for now, just use all the activities
          /*
          //if any of the query results are A -> hasActivity -> B, then B can't be in the list
          foreach ($query_results as $query_result) {
            $allpubpids[] = $query_result['pubpid']; //so a search can be done for the A->B example above
          }
          $remainingquery_result = $query_results;
          
          $resultingpubpids = $allpubpids;
          foreach ($query_results as $query_result) {
            $pubpid = $query_result['pubpid'];

            $fedora_object = islandora_object_load($pubpid);
            if ($fedora_object) {
              $rels = $fedora_object->relationships;
              $acts = $rels->get(FBA_URI, 'hasActivity');
              if (count($acts) > 0) {
                foreach ($acts as $act) {
                  $foundrelpid = $act[object][value];
                  if (in_array($foundrelpid, $allpubpids)) {
                    //this pid isn't a top level activity so won't be suitable for unpublishing
                    $key = array_search($foundrelpid, $resultingpubpids);
                    unset($remainingquery_result[$key]);
                  }
                }
              }
            }
          } */

          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'These are the pairings choices for unpublishing (objects with a doi are excluded)<br />',
          );
          if (count($query_results) > 0) {
            $rownumber = 0;
            foreach ($query_results as $result) {
              $pid = $result['pid'];
              $pubpid = $result['pubpid'];
              $reformattedpid = str_replace(':', '', $pubpid);
              $doiresult = $fba_doi->fba_check_doi($reformattedpid);
              if (!$doiresult) { //can't unpublish if a doi has been minted
                $form['row' . $rownumber] = array(
                  '#type' => 'fieldset',
                  '#collapsible' => FALSE,
                  '#collapsed' => FALSE,
                );
                $form['row' . $rownumber]['pid'] = array(
                  '#type' => 'markup',
                  '#value' => $pid . '->' . $pubpid . '<br/>',
                  '#parents' => array('row' . $rownumber),
                );
                if ($rownumber === 0) {
                  //force the first one to be checked... if value === return value, it gets checked automatically by the form code
                  $form['row' . $rownumber]['radiobutton'] = array(
                    '#type' => 'radio',
                    '#value' => $pid . '-' . $pubpid,
                    '#title' => $pid . '-' . $pubpid,
                    '#name' => 'pids',
                    '#return_value' => check_plain($pid . '-' . $pubpid),
                    '#parents' => array('row' . $rownumber),
                  );
                }
                else {
                  $form['row' . $rownumber]['radiobutton'] = array(
                    '#type' => 'radio',
                    '#title' => $pid . '-' . $pubpid,
                    '#name' => 'pids',
                    '#return_value' => check_plain($pid . '-' . $pubpid),
                    '#parents' => array('row' . $rownumber),
                  );
                }

                $rownumber++;
              }
            }
            $form['submit'] = array(
              '#type' => 'submit',
              '#value' => 'Go',
            );
          }
          else {
            $form['plaintext'] = array(
              '#type' => 'markup',
              '#value' => 'No candidates found for unpublishing.<br />',
            );
          }
        }
        else {
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'Islandora/FBA DOI configuration has missing value(s).<br />',
          );
        }

        break;
      }
    case 3: {
        $publishedpidstounpub = $_SESSION['pidstounpublish'];
        $selectedpubpid = $_SESSION['selectedpubpid'];
        $selectedpid = $_SESSION['selectedpid'];

        //if $publishedpidstounpub is empty, then $selectedpubpid doesn't exist, 
        //ie the relsext can be removed from $selectedpid
        if (count($publishedpidstounpub) === 0) {
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => $selectedpubpid . ' doesn\'t exist, so will remove hasPublishedVersion RDF from ' . $selectedpid . '<br />',
          );
          $_SESSION['rdfremove'][] = $selectedpid . '-' . $selectedpubpid;
          /*
            $form['rdfremove'] = array(
            '#type' => 'hidden',
            '#value' => check_plain($selectedpid . '-' . $selectedpubpid),
            ); */
        }
        else {
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => ' Things to do: <br />',
          );
          $i = 0;
          foreach ($publishedpidstounpub as $publishedpid) {
            $sposearchresult = _spo_unpub_search('*', '<http://fba.org.uk/schemas/fba/fba_rdf/1.0.0/hasPublishedVersion>', $publishedpid);
            if ($sposearchresult) {
              $private_objects[$publishedpid] = $sposearchresult[0];
              $form['plaintext'][] = array(
                '#type' => 'markup',
                '#value' => 'Remove hasPublishedVersion from ' . $private_objects[$publishedpid] . ' to ' . $publishedpid . ' <br />',
              );
              if ($private_objects[$publishedpid]) {
                $_SESSION['rdfremove'][] = $private_objects[$publishedpid] . '-' . $publishedpid;
                /*
                  $form['rdfremove'][$i] = array(
                  '#type' => 'hidden',
                  '#value' => check_plain($private_objects[$publishedpid] . '-' . $publishedpid),
                  ); */
              }
            }
            $i++;
          }
          $i = 0;
          foreach ($private_objects as $private_object) {
            $form['plaintext'][] = array(
              '#type' => 'markup',
              '#value' => 'Reset DTCA archivalPublicationState values back to null or 0 for ' . $private_object . ' <br />',
            );

            $_SESSION['resetdtca'][$i] = $private_object;
            /* $form['resetdtca'][$i] = array(
              '#type' => 'hidden',
              '#value' => check_plain($private_object),
              ); */
            $i++;
          }
          $i = 0;
          foreach ($publishedpidstounpub as $publishedpid) {
            $form['plaintext'][] = array(
              '#type' => 'markup',
              '#value' => 'Purge object ' . $publishedpid . ' <br />',
            );
            $_SESSION['purgeobject'][$i] = $publishedpid;
            /*
              $form['purgeobject'][$i] = array(
              '#type' => 'hidden',
              '#value' => check_plain($publishedpid),
              ); */
            $i++;
          }
        }
        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => 'Go',
        );
        unset($_SESSION['pidstounpublish']);
        unset($_SESSION['selectedpubpid']);
        unset($_SESSION['selectedpid']);

        break;
      }
    case 4: {
        unset($_SESSION['rdfremove']);
        unset($_SESSION['resetdtca']);
        unset($_SESSION['purgeobject']);

        break;
      }
    case 5: {
        break;
      }
    case 6: {
        break;
      }
    case 7: {
        break;
      }
    default: {
        break;
      }
  }

  return $form;
}

/**
 * This may be used in the future to check box the items for when not all objects
 * have been confirmed as correct or if there are multiple repo managers working
 * simultaneously
 * @param type $form
 */
function fba_repo_manager_multistep_unpublish_form_page($form) {
  /*
    if ($form['publishpid']) {
    $header = array(
    // This is the magic that creates the 'select all' box
    array('data' => t('Object Pid')),
    array('data' => t('Owner User Id')),
    theme('table_select_header_cell'),
    );

    // A variable to hold the row information for each table row
    $rows = array();

    // element_children filters out any form properties from an array
    // (i.e. items whose keys have a '#' before them) and leaves us
    // with an array of the leftover keys
    // So, this next bit will loop through the available nids that we're
    // using to numerate our form data and create render out the data
    // to our table. To get an idea of what your form data looks like
    // uncomment the following lines (make sure you have the devel module on!)
    // dpm($form);
    // dpm($form['title']);
    // dpm(element_children($form['title']));
    foreach (element_children($form['publishpid']) as $key => $value) {
    $pidstring = $form['publishpid'][$value]['#value'];
    $objectpid = substr($pidstring, 0, strpos($pidstring, '-'));
    $userid = substr($pidstring, strpos($pidstring, '-') + 1);
    $rows[] = array(//row
    'data' => array(//row data
    array(//Cell1
    // $form['nodes'] now contains individual checkbox form elements!
    'data' => $objectpid, //drupal_render($form['nodes'][$key]),
    'class' => '',
    ),
    array(//Cell2
    'data' => $userid, //drupal_render($form['title'][$key]),
    'class' => '',
    ),
    array(//Cell3
    'data' => 'SDF3', //drupal_render($form['created'][$key]),
    'class' => '',
    ),
    ), //endrow
    'class' => '', //row class
    ); //endrow
    }

    // With our data all formatted nicely, we can just use theme_table
    // To produce our table
    $output = theme('table', $header, $rows, array('class' => 'delete_nodes'));

    // The function drupal_render() Keeps track of all rendered elements
    // And makes sure it doesn't render the same item twice. Running
    // drupal_render on the form at the end just renders what's left:
    // the delete button
    }
    $output .= drupal_render($form);

    // And we return the output
    return $output;
   */
}

/**
 * hook submit
 * 
 * @param type $form
 * @param type $form_state
 */
function fba_repo_manager_multistep_unpublish_form_submit($form, &$form_state) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'content_model_viewer', 'Collection');
  module_load_include('inc', 'sidora', 'includes/fba');

  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;
  switch ($step) {
    case 1: {
        break;
      }
    case 2: {
        $selectedradiobutton = filter_input(INPUT_POST, 'pids');
        $selectedpid = substr($selectedradiobutton, 0, strpos($selectedradiobutton, '-'));
        $selectedpubpid = substr($selectedradiobutton, strpos($selectedradiobutton, '-') + 1);
        $_SESSION['pidstounpublish'] = pubsearch($selectedpubpid);
        $_SESSION['selectedpid'] = $selectedpid;
        $_SESSION['selectedpubpid'] = $selectedpubpid;

        break;
      }
    case 3: {
        $rdfremove = $_SESSION['rdfremove'];
        $resetdtca = $_SESSION['resetdtca'];
        $purgeobject = $_SESSION['purgeobject'];
        $batch = _unpublish_batch_1($rdfremove, $resetdtca, $purgeobject);
        batch_set($batch);
        break;
      }
    case 4: {
        break;
      }
    case 5: {
        break;
      }
    case 6: {
        break;
      }
    case 7: {
        break;
      }
  }

  $form_state['storage']['step'] = $step + 1;
}

/**
 * This function registers themeing functions with Drupal.
 *
 * @return array
 *   $themes the array of available themes
 */
function fba_repo_manager_multistep_unpublish_form_theme() {
  $path = drupal_get_path('module', 'islandora_workflow');
  $themes = array();
  $themes['permissions_role_table'] = array(
    'arguments' => array('list' => NULL, 'form' => NULL),
    'template' => 'permissions_role_table',
    'path' => "$path /theme",
    'file' => "islandora_workflow.theme.inc",
  );
  $themes['permissions_user_table'] = array(
    'arguments' => array('list' => NULL, 'form' => NULL),
    'template' => 'permissions_user_table',
    'path' => "$path /theme",
    'file' => "islandora_workflow.theme.inc",
  );
  // Both workflow and overview tabs should be able to use this template.
  $themes['workflow_table'] = array(
    'arguments' => array('form' => NULL),
    'path' => "$path /theme",
    'file' => "islandora_workflow.theme.inc",
  );
  // Both workflow and overview tabs should be able to use this template.
  $themes['workflow_collections'] = array(
    'arguments' => array('list' => NULL, 'form' => NULL),
    'template' => 'workflow_collections',
    'path' => "$path /theme",
    'file' => "islandora_workflow.theme.inc",
  );
  return $themes;
}

/**
 * 
 * @return array
 */
function _unpublish_batch_1($rdfremove, $resetdtca, $purgeobject) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');

  $operations = array();

  if (count($rdfremove) > 0) {
    foreach ($rdfremove as $rdfstring) {
      $leftpid = substr($rdfstring, 0, strpos($rdfstring, '-'));
      $rightpid = substr($rdfstring, strpos($rdfstring, '-') + 1);
      $operations[] = array('_rdfremove', array($leftpid, FBA_URI, 'hasPublishedVersion', $rightpid));
    }
  }

  if (count($resetdtca) > 0) {
    foreach ($resetdtca as $dtcapid) {
      $operations[] = array('_resetdtca', array($dtcapid));
    }
  }

  if (count($purgeobject) > 0) {
    foreach ($purgeobject as $purgepid) {
      $operations[] = array('_purgeobject', array($purgepid));
    }
  }
  // Create an array which contains an array of the operations to
  // perform and a method to call when the operations are all finished

  $batch = array(
    'operations' => $operations,
    'finished' => '_unpublish_finished_1',
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/batchfunctions_unpublish.inc',
    'title' => t('Process object, and apply unpublish workflows'),
    'init_message' => t('Initialising...'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('_unpublish_batch_1 encountered an error.'),
  );
  return $batch;
}

/**
 * 
 * @return array
 */
function _unpublish_batch_3($topactivitypid, $clonelist) {
  global $user;

  $batch = array(
    'title' => t('First batch run'),
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/batchfunctions_unpublish.inc',
    'operations' => array(),
    'finished' => '_unpublish_finished_3',
  );
  //enumerate through clone list to create new objects
  foreach ($clonelist as $clonepid) {
    if (strcmp($topactivitypid, $clonepid) === 0) {
      //array_unshift($batch['operations'], array('cloneobject', array($topactivitypid, TRUE))); //TRUE as it's the root object being submitted
      array_unshift($batch['operations'], array('cloneobject', array($topactivitypid))); //TRUE as it's the root object being submitted
    }
    else {
      //array_unshift($batch['operations'], array('cloneobject', array($clonepid, FALSE)));
      array_unshift($batch['operations'], array('cloneobject', array($clonepid)));
    }
  }
  return

      $batch;
}

function _unpublish_batch_4($pids) {
  global $user;

  $batch = array(
    'title' => t('Second batch run'),
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/batchfunctions_unpublish.inc',
    'operations' => array(),
    'finished' => '_unpublish_finished_4',
  );
  //populate the DTCA observed properties from the csv files (data components only)
  if (count($pids) > 0) {
    foreach ($pids as $pid) {
      if (sidora_is_datacomponent($pid[0])) {
        array_push($batch['operations'], array('updatedtca', array($pid[1])));
      }
    }
  }

  $batch['operations'][] = array('tidyuprelations', array($pids));
  return $batch;
}

/**
 * Batch operation for batch 5
 * 
 * @param type $pid
 * @param type $context
 */
function _unpublish_batch_5($pids) {
  $batch = array(
    'title' => t('Third batch run'),
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/batchfunctions_unpublish.inc',
    'operations' => array(),
    'finished' => '_unpublish_finished_5',
  );

  if (count($pids) > 0) {
    foreach ($pids as $pid) {
      $batch['operations'][] = array(
        'downloadablefilesizes', array($pid[1]));
    }
  }
  return $batch;
}

/**
 * Batch operation for batch 6
 * 
 * @param type $pid
 * @param type $context
 */
function _unpublish_batch_6($pids) {
  $batch = array(
    'title' => t('Fourth batch run'),
    'file' => drupal_get_path('module', 'fba_doi') . '/batchfunctions.inc',
    'operations' => array(),
    'finished' => '_unpublish_finished_6',
  );
  foreach ($pids as $pid) {
    $batch['operations'][] = array('batchmintdoi', array($pid[1]));
  }
  return $batch;
}

/**
 * Batch operation for batch 1
 *
 * @param type $memberpid
 * @param type $context
 */
function _unpublish_op_1($memberpid, &$context) {
  module_load_include('inc', 'fedora_repository', 'api/tuque');
  module_load_include('inc', 'sidora', 'includes/fba');

  $fedora_object = islandora_object_load($memberpid);
  $rels = $fedora_object->relationships;
  $imoc = $rels->get(FEDORA_RELS_EXT_URI, 'isMemberOfCollection');
  $imocname = $imoc[0]['object']['value'];
  $imocprefix = substr($imocname, 0, strpos($imocname, ':'));
  $resource_rdfs = sidora_return_fba_resource_rdfs();
  $concept_rdfs = sidora_return_fba_concept_rdfs();
  $rdfs = array_merge($resource_rdfs, $concept_rdfs);
  foreach ($rdfs as $rdfkey => $rdfname) {
    $objectrel = $rels->get(FEDORA_RELS_EXT_URI, $rdfname);
    if (count($objectrel) > 0) {
      $objectrelname = $objectrel[0]['object']['value'];
    }
  }
  $dtca = $fedora_object['DTCA'];
  $dtcacontent = $dtca->content;
  $dtcads = simplexml_load_string($dtcacontent);
  $dtcads->registerXPathNamespace('dtca', 'http://fba.org.uk/schemas/dtca/1.0.0');
  $pubxpath = $dtcads->xpath('//dtca:archivalPublicationState');
  if (count($pubxpath) > 0) {
    $pubxpathvalue = (string) $pubxpath[0];
    $context['results'][$pubxpathvalue][] = array($imocprefix, $memberpid, $objectrelname, $pubxpathvalue);
  }
  // Optional message displayed under the progressbar.
  $context['message'] = t('Working on @pid', array('@pid' => $memberpid));
}

/**
 * Batch 'finished' callback 
 * @param type $success
 * @param type $results
 * @param type $operations
 */
function _unpublish_finished_1($success, $results, $operations) {
  if ($success) {
    // Here we could do something meaningful with the results.
    // We just display the number of nodes we processed...
    $rdfsremoved = $results['rdfremove'];
    $resetdtca = $results['resetdtca'];
    $purgedobjects = $results['purgeobject'];
    if (count($rdfsremoved) > 0) {
      foreach ($rdfsremoved as $rdfremovedpid => $rdfremovedarray) {
        foreach ($rdfremovedarray as $rdfkey => $rdfvalue) {
          if ($rdfvalue === TRUE) {
            $rdftext = 'true';
          }
          else {
            $rdftext = 'false';
          }
          $message .= 'rdfremoved for pid ' . $rdfremovedpid . '->' . $rdfkey . '->' . $rdftext . '<br/>';
        }
      }
    }

    if (count($resetdtca) > 0) {
      foreach ($resetdtca as $dtcapid => $dtcapidstring) {
        $message .= 'dtca fields reset for pid ' . $dtcapid . '->' . $dtcapidstring . '<br/>';
      }
    }
    if (count($purgedobjects) > 0) {
      foreach ($purgedobjects as $purgedobjectpid => $purgedobjectbool) {
        if ($purgedobjectbool === TRUE) {
          $pobtext = 'true';
        }
        else {
          $pobtext = 'false';
        }
        $message .= 'object purged for pid ' . $purgedobjectpid . '->' . $pobtext . '<br/>';
      }
    }
  }
  else {
    // An error occurred.
    $message = 'An error occurred while processing phase 1.';
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
function _unpublish_finished_3($success, $results, $operations) {
  $succeeded = TRUE;
  foreach ($results['success'] as $ingestresult) {
    //check if any of the ingest steps failed
    if (!$ingestresult[2]) {
      $succeeded = FALSE;
    }
  }
  if ($succeeded) {
    $message = 'Phase 3 completed successfully.';
    $_SESSION['my_batch_succeeded'] = TRUE;
    $_SESSION['my_batch_results'] = $results['success'];
  }
  else {
    // An error occurred.
    $message = 'An error occurred while processing phase 3.';
    $_SESSION['my_batch_succeeded'] = FALSE;
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
function _unpublish_finished_4($success, $results, $operations) {
  if ($success) {
    $message = 'Phase 4 completed successfully.';
    $_SESSION['my_batch_succeeded'] = TRUE;
    $_SESSION['my_batch_results'] = $results;
  }
  else {
    // An error occurred.
    $message = 'An error occurred while processing phase 4.';
    $_SESSION['my_batch_succeeded'] = FALSE;
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
function _unpublish_finished_5($success, $results, $operations) {

  if ($success) {
    $message = 'Objects have had been updated with filesizes.<br/>';
  }
  else {
    $message = 'An error occurred while processing phase 5.';
    $_SESSION['my_batch_succeeded'] = FALSE;
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
function _unpublish_finished_6($success, $results, $operations) {
  $succeeded = TRUE;
  foreach ($results['success'] as $mintresult) {
    //check if any of the ingest steps failed
    if (!$mintresult[1]) {
      $succeeded = FALSE;
    }
  }

  if ($succeeded) {
    $message = count($results ['success']) . ' objects have had DOIs minted.<br/>';
    foreach ($results['success'] as $result) {
      $message .= 'PID: ' . $result[0] . '<br/>';
    }
  }
  else {
    $message = 'An error occurred while processing phase 5.';
    $_SESSION['my_batch_succeeded'] = FALSE;
  }
  drupal_set_message(
      $message);
}

function list_all_publication_collections() {
  return array(
    'dtcavon:collection',
    'dtceden:collection',
    'dtcwensum:collection',
    'fbads:collection',
    'syngds:collection'
  );
}

/*
  SELECT ?pid ?pubpid WHERE
  {
  ?pid ?predicate ?object .
  ?pid <fedora-model:state> <fedora-model:Active> .
  ?pid <fedora-model:hasModel> <info:fedora/fba:activity_cmodel> .
  ?pid <http://fba.org.uk/schemas/fba/fba_rdf/1.0.0/hasPublishedVersion> ?pubpid.
  ?pid <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/dtcwensum:collection>
  }
 * 
 */

function _spo_unpub_search($subject = '*', $predicate = '*', $object = '*') {
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
  $query = $subject . ' ' . $predicate . ' <info:fedora/' . $object . '>';
  $offset = 0;
  $limit = 10000000;

  $encodedquery = htmlentities(urlencode($query));

  $url = variable_get('fedora_repository_url', 'http://localhost:8080/fedora/risearch');
  $url .= "?type=triples&flush=TRUE&format=N-Triples&limit=$limit&offset=$offset&lang=spo&stream=on&query=" . $encodedquery;
  $content = trim(do_curl($url));
  $preds = array_merge(sidora_return_fba_classes_namespaces(), sidora_return_fba_csv_resources_namespaces());
  $preds[] = 'fba-user'; //Quickest way of including this in the search

  if ($content) {
    $content = str_replace("\n", '', $content);
    $triples = explode(" .", $content);
    $tripleCount = count($triples) - 1;
    $subjectElement = 0;
    $processedTriples = array();
    for ($i = 0; $i < $tripleCount; $i++) {
      $elements = explode("> ", $triples[$i]);
      $pid = substr($elements[$subjectElement], strlen('<info:fedora/'));
      $pidns = substr($pid, 0, strpos($pid, ':'));
      if (in_array($pidns, $preds)) {
        $processedTriples[] = $pid;
      }
    }
    return $processedTriples;
  }
  else {
    return NULL;
  }
}
