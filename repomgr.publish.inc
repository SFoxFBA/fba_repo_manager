<?php

/**
 * Multistep form
 * @param type $form_state
 * @return string
 * 
 */
function fba_repo_manager_multistep_publish_form($form_state = NULL) {
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('module', 'fba_doi', 'fba_doi');

  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;
  // This is what tells the form which theme function to use
  /* $form['step_display'] = array(
    '#type' => 'item',
    '#value' => 'step ' . $step,
    ); */
  switch ($step) {
    case 1: {
        //check that doi has been configured with some values
        $fba_doi = new fba_doi();
        if ($fba_doi->fba_check_doi_configuration()) {
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'This step searches for datasets with a submitted tag in the DTCA datastream <br />',
          );
          $form['submit'] = array(
            '#type' => 'submit',
            '#value' => 'Go',
          );
        }
        else {
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'Islandora DOI configuration has missing value(s).<br />',
          );
        }

        break;
      }
    case 2: {
        //find the submittedtop entries
        //needed? $form['#theme'] = 'fba_repo_manager_multistep_publish_form';

        $pidswithsubmittedflagarray = $_SESSION['submittedtop'];
        if ($pidswithsubmittedflagarray) {

          usort($pidswithsubmittedflagarray, 'sortsubmittedpids');
          $rownumber = 0;
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'This step shows the activity of each submitted branch<br />',
          );
          foreach ($pidswithsubmittedflagarray as $pidwithsubmittedflag) {
            $rownumber++;

            /* use this if the fba_repo_manager_multistep_publish_form_page is active in the future 
              $form['publishpid'][] = array(
              '#type' => 'markup',
              '#value' => $pidwithsubmittedflag[1] .'-'. $pidwithsubmittedflag[1],
              ); */
            $form['row' . $rownumber] = array(
              '#type' => 'fieldset',
              '#collapsible' => FALSE,
              '#collapsed' => FALSE,
            );
            $form['row' . $rownumber]['publishpid'] = array(
              '#type' => 'markup',
              '#value' => 'Top level activity pid:' . $pidwithsubmittedflag[1] . '<br/>',
            );
            $form['row' . $rownumber]['ownerid'] = array(
              '#type' => 'markup',
              '#value' => 'Owner Id: ' . $pidwithsubmittedflag[0] . '<br/>',
            );
            $form['row' . $rownumber]['radiobutton'] = array(
              '#type' => 'radio',
              '#value' => $pidwithsubmittedflag[1] . '-' . $pidwithsubmittedflag[0],
              '#title' => $pidwithsubmittedflag[1] . ' title',
              '#name' => 'pids',
              '#return_value' => check_plain($pidwithsubmittedflag[1] . '-' . $pidwithsubmittedflag[0]),
              '#parents' => array('row' . $rownumber),
            );
            $form['row' . $rownumber]['hyperlink'] = array(
              '#type' => 'markup',
              '#value' => '<a href="http://environmentdata.org/archive/' . $pidwithsubmittedflag[1] . '" target="#blank" >' . $pidwithsubmittedflag[1] . '</a>',
            );
          }
        }
        if ($rownumber > 0) {
          $form['submit'] = array(
            '#type' => 'submit',
            '#value' => 'Open the object with selected radio button',
          );
        }
        else {
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'No activities submitted for publication.<br />',
          );
        }
        break;
      }
    case 3: {
        $rownumber = 0;
        $form['plaintext'] = array(
          '#type' => 'markup',
          '#value' => 'This step shows the descendants of the chosen submitted activity<br />',
        );

        foreach ($_SESSION['pidstopublish'] as $pidtopublish) {
          $rownumber++;

          $form['row' . $rownumber] = array(
            '#type' => 'fieldset',
            '#collapsible' => FALSE,
            '#collapsed' => FALSE,
          );
          $form['row' . $rownumber]['publishpid'] = array(
            '#type' => 'markup',
            '#value' => '<a href="http://environmentdata.org/archive/' . $pidtopublish . '" target="#blank" >' . $pidtopublish . '</a>',
          );
        }
//        $form['pidstopublish'] = array(
//          '#type' => 'hidden',
//          '#value' => $form_state['values']['pidstopublish'], // TODO: arrays aren't supported well in forms in hidden field!
//        );
//        $form['selectedpid'] = array(
//          '#type' => 'hidden',
//          '#value' => $form_state['values']['selectedpid'],
//        );
//        $form['selectedowner'] = array(
//          '#type' => 'hidden',
//          '#value' => $form_state['values']['selectedowner'],
//        );

        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => 'Publish all the above objects',
        );
        //$_SESSION['pidstopublish'] = $form_state['values']['pidstopublish'];
        //$_SESSION['selectedpid'] = $form_state['values']['selectedpid'];
        //$_SESSION['selectedowner'] = $form_state['values']['selectedowner'];

        break;
      }
    case 4: {
        $rownumber = 0;
        $form['plaintext'] = array(
          '#type' => 'markup',
          '#value' => 'This step triggers tidying of the datastreams for the published objects<br />',
        );

        foreach ($_SESSION['pidstopublish'] as $pidtopublish) {
          $form['row' . $rownumber] = array(
            '#type' => 'fieldset',
            '#collapsible' => FALSE,
            '#collapsed' => FALSE,
          );
          $form['row' . $rownumber]['publishpid'] = array(
            '#type' => 'markup',
            '#value' => '<a href="http://environmentdata.org/archive/' . $pidtopublish . '" target="#blank" >' . $pidtopublish . '</a>',
          );
          $rownumber++;
        }
//        $form['pidstopublish'] = array(
//          '#type' => 'hidden',
//          '#value' => $form_state['values']['pidstopublish'], // TODO: arrays aren't supported well in forms in hidden field!
//        );
//        $form['selectedpid'] = array(
//          '#type' => 'hidden',
//          '#value' => $form_state['values']['selectedpid'],
//        );
//        $form['selectedowner'] = array(
//          '#type' => 'hidden',
//          '#value' => $form_state['values']['selectedowner'],
//        );

        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => 'Correct the datastreams of all the above objects',
        );
        //$_SESSION['pidstopublish'] = $form_state['values']['pidstopublish'];
        //$_SESSION['selectedpid'] = $form_state['values']['selectedpid'];
        //$_SESSION['selectedowner'] = $form_state['values']['selectedowner'];

        break;
      }
    case 5: {
        $rownumber = 0;
        $form['plaintext'] = array(
          '#type' => 'markup',
          '#value' => 'This step adds individual and cumulative filesizes to objects<br />',
        );
        if ($_SESSION['my_batch_results']['success']) {
          foreach ($_SESSION['my_batch_results']['success'] as $pidtoupdate) {
            if (sidora_is_datacomponent($pidtoupdate[0]) || sidora_is_dataset($pidtoupdate[0])) {
              //original private object was a dataset or a datacomponent, so filesize updates required

              $form['row' . $rownumber] = array(
                '#type' => 'fieldset',
                '#collapsible' => FALSE,
                '#collapsed' => FALSE,
              );
              $form['row' . $rownumber]['publishpid'] = array(
                '#type' => 'markup',
                '#value' => '<a href="http://environmentdata.org/archive/' . $pidtoupdate[1] . '" target="#blank" >' . $pidtoupdate[1] . '</a>',
              );
              $rownumber++;
            }
          }
        }
        else {
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'No PIDS returned from previous phase<br />',
          );
        }

        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => 'Disseminates filesizes across objects',
        );

        break;
      }
    case 6: {
        $rownumber = 0;
        $form['plaintext'] = array(
          '#type' => 'markup',
          '#value' => 'This step triggers DOI minting for the published datasets<br />',
        );
        if ($_SESSION['my_batch_results']['success']) {
          foreach ($_SESSION['my_batch_results']['success'] as $pidtoupdate) {
            if (strcmp(substr($pidtoupdate[0], 0, strpos($pidtoupdate[0], ':')), 'dtcds') === 0) {
              //original private object was a dataset, so a DOI is required

              $form['row' . $rownumber] = array(
                '#type' => 'fieldset',
                '#collapsible' => FALSE,
                '#collapsed' => FALSE,
              );
              $form['row' . $rownumber]['publishpid'] = array(
                '#type' => 'markup',
                '#value' => '<a href="http://environmentdata.org/archive/' . $pidtoupdate[1] . '" target="#blank" >' . $pidtoupdate[1] . '</a>',
              );
              $rownumber++;
            }
          }
        }
        else {
          $form['plaintext'] = array(
            '#type' => 'markup',
            '#value' => 'No PIDS returned from previous phase<br />',
          );
        }
//        $form['pidstopublish'] = array(
//          '#type' => 'hidden',
//          '#value' => $form_state['values']['pidstopublish'], // TODO: arrays aren't supported well in forms in hidden field!
//        );
//        $form['selectedpid'] = array(
//          '#type' => 'hidden',
//          '#value' => $form_state['values']['selectedpid'],
//        );
//        $form['selectedowner'] = array(
//          '#type' => 'hidden',
//          '#value' => $form_state['values']['selectedowner'],
//        );

        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => 'Mint DOIs for all the datasets',
        );
        //$_SESSION['pidstopublish'] = $form_state['values']['pidstopublish'];
        //$_SESSION['selectedpid'] = $form_state['values']['selectedpid'];
        //$_SESSION['selectedowner'] = $form_state['values']['selectedowner'];

        break;
      }
    default: {
        break;
      }
  }

  return $form;
}

/**
 * Custom sort using the 2nd value in the array
 * 
 * @param array $a
 * @param array $b
 * @return int
 */
function sortsubmittedpids($a, $b) {
  if ($a[1] === $b[1]) {
    return 0;
  }
  return ($a[1] < $b[1]) ? -1 : 1;
}

/**
 * This may be used in the future to check box the items for when not all objects
 * have been confirmed as correct or if there are multiple repo managers working
 * simultaneously
 * @param type $form
 */
function fba_repo_manager_multistep_publish_form_page($form) {
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
function fba_repo_manager_multistep_publish_form_submit($form, &$form_state) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'content_model_viewer', 'Collection');
  module_load_include('inc', 'sidora', 'includes/fba');

  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;
  switch ($step) {
    case 1: {
        //drupal_set_message('step 1 completed');
        batch_set(_publish_batch_1());
        break;
      }
    case 2: {
        $selectedradiobutton = filter_input(INPUT_POST, 'pids');
        $selectedpid = substr($selectedradiobutton, 0, strpos($selectedradiobutton, '-'));
        $selectedowner = substr($selectedradiobutton, strpos($selectedradiobutton, '-') + 1);
//        $form_state['values']['pidstopublish'] = pubsearch($selectedpid); //recursive search of all children
//        $form_state['values']['selectedpid'] = $selectedpid;
//        $form_state['values']['selectedowner'] = $selectedowner;
        $_SESSION['pidstopublish'] = pubsearch($selectedpid);
        $_SESSION['selectedpid'] = $selectedpid;
        $_SESSION['selectedowner'] = $selectedowner;

        break;
      }
    case 3: {
        $pidstopublish = $_SESSION['pidstopublish'];
        $selectedpid = $_SESSION['selectedpid'];
        batch_set(_publish_batch_3($selectedpid, $pidstopublish));
        break;
      }
    case 4: {
        //Another batch job is required here to swap all the old and new pids in the new objects
        $batchresultsuccess = $_SESSION['my_batch_succeeded']; //check return results
        $batchresult = $_SESSION['my_batch_results']; //check return results

        if ($batchresultsuccess) { //previous step worked
          $batch = _publish_batch_4($batchresult);
          batch_set($batch);
        }
        break;
      }
    case 5: {
        $batchresultsuccess = $_SESSION['my_batch_succeeded']; //check return results
        $batchresult = $_SESSION['my_batch_results'];
        if ($batchresultsuccess) { //previous step worked
          $batch = _publish_batch_5($batchresult['success']);
          batch_set($batch);
        }
        break;
      }
    case 6: {
        $batchresultsuccess = $_SESSION['my_batch_succeeded']; //check return results
        $batchresult = $_SESSION['my_batch_results'];
        if ($batchresultsuccess) { //previous step worked
          $batch = _publish_batch_6($batchresult['success']);
          batch_set($batch);
        }
        break;
      }
  }

  $form_state['storage']['step'] = $step + 1;
}

function pubsearch($pid) {
  module_load_include('inc', 'sidora', 'includes/fba');

  $results = array();
  //get children from rels-ext
  $rdfs = array_merge(sidora_return_fba_concept_rdfs(), sidora_return_fba_resource_rdfs());
  pubsearch_r($pid, $rdfs, $results);

  return $results;
}

function pubsearch_r($pid, $rdfs, &$results) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/fba');

  $repository = tuque_wrapper_get_repository_instance();
  try {
    $objecttopublish = $repository->getObject($pid);
  }
  catch (RepositoryException $ex) {
    $code = $ex->getCode(); //The object may not exist
  }
  if ($objecttopublish) {
    $results[] = $pid;
    foreach ($rdfs as $rdf) {
      $objrels = $objecttopublish->relationships->get(FBA_URI, $rdf);
      if (count($objrels) > 0) {
        foreach ($objrels as $objrel) {
          $nextpid = $objrel['object']['value'];
          pubsearch_r($nextpid, $rdfs, $results);
        }
      }
    }
  }
}

/**
 * This function registers themeing functions with Drupal.
 *
 * @return array
 *   $themes the array of available themes
 */
function fba_repo_manager_multistep_publish_form_theme() {
  $path = drupal_get_path('module', 'islandora_workflow');
  $themes = array();
  $themes['permissions_role_table'] = array(
    'arguments' => array('list' => NULL, 'form' => NULL),
    'template' => 'permissions_role_table',
    'path' => "$path/theme",
    'file' => "islandora_workflow.theme.inc",
  );
  $themes['permissions_user_table'] = array(
    'arguments' => array('list' => NULL, 'form' => NULL),
    'template' => 'permissions_user_table',
    'path' => "$path/theme",
    'file' => "islandora_workflow.theme.inc",
  );
  // Both workflow and overview tabs should be able to use this template.
  $themes['workflow_table'] = array(
    'arguments' => array('form' => NULL),
    'path' => "$path/theme",
    'file' => "islandora_workflow.theme.inc",
  );
  // Both workflow and overview tabs should be able to use this template.
  $themes['workflow_collections'] = array(
    'arguments' => array('list' => NULL, 'form' => NULL),
    'template' => 'workflow_collections',
    'path' => "$path/theme",
    'file' => "islandora_workflow.theme.inc",
  );
  return $themes;
}

/**
 * 
 * @return array
 */
function _publish_batch_1() {
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');

  $operations = array();

  //$rdfns = array_merge(sidora_return_fba_classes_namespaces(), sidora_return_fba_resources_namespaces());
  $rdfns[] = 'dtcac';
  $query_string = "select DISTINCT ?object where {?object ?y ?z . FILTER regex(str(?object),";
  //"dtcmt|dtcac|dtcds" 
  $arraycount = count($rdfns);
  foreach ($rdfns as $key => $keyvalue) {
    $allns .= $keyvalue;
    if ($key < $arraycount - 1) { //as arraycounts don't start at zero
      $allns .= "|";
    }
  }
  $query_string .= "'" . $allns . "'). FILTER regex(str(?object), 'DTCA')}";
  $query_result = ObjectHelper::performRiQuery($query_string, 'sparql');

  foreach ($query_result as $result) {
    if ($result['object']) {
      //chop /DTCA off the end of the string to leave pid
      $operations[] = array('_publish_op_1', array(substr($result['object'], 0, strpos($result['object'], '/'))));
    }
  }

  // Create an array which contains an array of the operations to
  // perform and a method to call when the operations are all finished
  $batch = array(
    'operations' => $operations,
    'finished' => '_publish_finished_1',
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/repomgr.publish.inc',
    'title' => t('Process DTCA to find objects with submitted tag'),
    'init_message' => t('Initialising...'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('_publish_batch_1 encountered an error.'),
  );
  return $batch;
}

/**
 * This sets up the list of objects to be cloned
 * @return array
 */
function _publish_batch_3($topactivitypid, $clonelist) {
  $batch = array(
    'title' => t('First batch run'),
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/batchfunctions_publish.inc',
    'operations' => array(),
    'finished' => '_publish_finished_3',
  );
  //enumerate through clone list to create new objects
  foreach ($clonelist as $clonepid) {
    array_unshift($batch['operations'], array('cloneobject', array($clonepid)));
  }
  return $batch;
}

function _publish_batch_4($pids) {
  $batch = array(
    'title' => t('Second batch run'),
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/batchfunctions_publish.inc',
    'operations' => array(),
    'finished' => '_publish_finished_4',
  );
  //populate the DTCA observed properties from the csv files (data components only)
  if (count($pids) > 0) {
    foreach ($pids as $pidarray) {
      if (sidora_is_datacomponent($pidarray[0])) {
        $batch['operations'][] = array('updatedtca', array($pidarray[1]));
      }
    }
  }

  if (count($pids) > 0) {
    foreach ($pids as $pidarray) {
      $batch['operations'][] = array('tidyuprelation', array($pids, $pidarray)); //pids is before/after list, pid is current object being processed
    }
  }
  return $batch;
}

/**
 * Batch operation for batch 5
 * 
 * @param type $pid
 * @param type $context
 */
function _publish_batch_5($pids) {
  module_load_include('inc', 'sidora', 'includes/fba');

  $batch = array(
    'title' => t('Third batch run'),
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/batchfunctions_publish.inc',
    'operations' => array(),
    'finished' => '_publish_finished_5',
  );

  if (count($pids) > 0) {
    foreach ($pids as $pid) {
      if (sidora_is_dataset($pid[0])) {
        $batch['operations'][] = array(
          'downloadablefilesizes', array($pid[0], $pid[1]));
      }
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
function _publish_batch_6($pids) {
  $batch = array(
    'title' => t('Fourth batch run'),
    'file' => drupal_get_path('module', 'fba_doi') . '/batchfunctions.inc',
    'operations' => array(),
    'finished' => '_publish_finished_6',
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
function _publish_op_1($memberpid, &$context) {
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
function _publish_finished_1($success, $results, $operations) {
  if ($success) {
    // Here we could do something meaningful with the results.
    // We just display the number of nodes we processed...
    $message = count($results['submittedtop']) . ' branches have been submitted.<br/>';
    $_SESSION['submittedtop'] = $results['submittedtop'];
    $_SESSION['submitted'] = $results['submitted'];
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
function _publish_finished_3($success, $results, $operations) {
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
function _publish_finished_4($success, $results, $operations) {
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
function _publish_finished_5($success, $results, $operations) {

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
function _publish_finished_6($success, $results, $operations) {
  $succeeded = TRUE;
  foreach ($results['success'] as $mintresult) {
    //check if any of the ingest steps failed
    if (!$mintresult[1]) {
      $succeeded = FALSE;
    }
  }

  if ($succeeded) {
    $message = count($results['success']) . ' objects have had DOIs minted.<br/>';
    foreach ($results['success'] as $result) {
      $message .= 'PID: ' . $result[0] . '<br/>';
    }
  }
  else {
    $message = 'An error occurred while processing phase 5.';
    $_SESSION['my_batch_succeeded'] = FALSE;
  }
  drupal_set_message($message);
}
