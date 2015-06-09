<?php

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
function _publish_finished_2($success, $results, $operations) {
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
 * 
 */
function fba_repo_manager_multistep_publish_form($form_state = NULL) {

  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] : 1;
// This is what tells the form which theme function to use
  /* $form['step_display'] = array(
    '#type' => 'item',
    '#value' => 'step ' . $step,
    ); */
  if ($step === 1) {
    $form['plaintext'] = array(
      '#type' => 'markup',
      '#value' => 'This step searches for datasets with a submitted tag in the DTCA datastream <br />',
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Go',
    );
  }
  if ($step === 2) {
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
          '#value' => '<a href="http://10.0.10.202/environmentdata.local/archive/' . $pidwithsubmittedflag[1] . '" target="#blank" >' . $pidwithsubmittedflag[1] . '</a>',
        );
        $rownumber++;
      }
    }
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Open the object with selected radio button',
    );
  }
  if ($step === 3) {
    $rownumber = 0;
    $form['plaintext'] = array(
      '#type' => 'markup',
      '#value' => 'This step shows the descendants of the chosen submitted activity<br />',
    );

    foreach ($form_state['values']['pidstopublish'] as $pidtopublish) {
      $form['row' . $rownumber] = array(
        '#type' => 'fieldset',
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
      );
      $form['row' . $rownumber]['publishpid'] = array(
        '#type' => 'markup',
        '#value' => '<a href="http://10.0.10.202/environmentdata.local/archive/' . $pidtopublish . '" target="#blank" >' . $pidtopublish . '</a>',
      );
      $rownumber++;
    }
    $form['pidstopublish'] = array(
      '#type' => 'hidden',
      '#value' => $form_state['values']['pidstopublish'], // TODO: arrays aren't supported well in forms in hidden field!
    );
    $form['selectedpid'] = array(
      '#type' => 'hidden',
      '#value' => $form_state['values']['selectedpid'],
    );
    $form['selectedowner'] = array(
      '#type' => 'hidden',
      '#value' => $form_state['values']['selectedowner'],
    );

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Publish all the above objects',
    );
  }

  return $form;
}

function sortsubmittedpids($a, $b) {
  if ($a[1] === $b[1]) {
    return 0;
  }
  return ($a[1] < $b[1]) ? -1 : 1;
}

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
        $form_state['values']['pidstopublish'] = pubsearch($selectedpid);
        $form_state['values']['selectedpid'] = $selectedpid;
        $form_state['values']['selectedowner'] = $selectedowner;
        break;
      }
    case 3: {
        $batch = _publish_batch_3($form_state['values']['selectedpid'], $form_state['values']['pidstopublish']);
        batch_set($batch);
        $batchtidy = _publish_batch_3_tidy();
        batch_set($batchtidy);
        $debugstring = '';
        foreach ($batchresult['success'] as $result) {
          $debugstring .= $result[0] . ' -> ' . $result[1] . ' -> ' . $result[2] . '<br/>';
        }

        break;
      }
  }

  $form_state['storage']['step'] = $step + 1;
}

function pubsearch($pid) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/fba');

  $repository = tuque_wrapper_get_repository_instance();

  $results = array();
  $objecttopublish = $repository->getObject($pid);
  //get children from rels-ext
  $rdfs = array_merge(sidora_return_forward_fba_concept_rdfs(), sidora_return_forward_fba_resource_rdfs());
  pubsearch_r($pid, $rdfs, $results);

  return $results;
}

function pubsearch_r($pid, $rdfs, &$results) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/fba');

  $repository = tuque_wrapper_get_repository_instance();
  $objecttopublish = $repository->getObject($pid);

  $results[] = $pid;
  foreach ($rdfs as $rdf) {
    $objrels = $objecttopublish->relationships->get(FEDORA_RELS_EXT_URI, $rdf);
    if ($objrels) {
      foreach ($objrels as $objrel) {
        $nextpid = $objrel['object']['value'];
        pubsearch_r($nextpid, $rdfs, $results);
      }
    }
  }
}

/**
 * Batch 1
 *
 * This method does not yet do the work; it simply builds an array
 * of the work which needs to be done during the batch processing.
 * The results of this function will be passed to the batch processor
 * for actual processing.
 */
function _publish_batch_1() {
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');

  $operations = array();

  $rdfns = array_merge(sidora_return_fba_classes_namespaces(), sidora_return_fba_resources_namespaces());

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
      $resultpid = substr($result['object'], 0, strpos($result['object'], '/'));
    }

    $all_pids[] = $resultpid;
  }

  foreach ($all_pids as $pid) {
    $operations[] = array('_publish_op_1', array($pid));
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
 * Batch operation for batch 1
 *
 * @param type $memberpid
 * @param type $context
 */
function _publish_op_1($memberpid, &$context) {
  module_load_include('inc', 'fedora_repository', 'api/tuque');
  module_load_include('inc', 'sidora', 'includes/fba');

// get the mads datastream for this object
//  $fedora_object = islandora_object_load($memberpid);
//  $dtca = $fedora_object['DTCA'];
//  $dtcacontent = $dtca->content;
//  $sxmldtca = new SimpleXMLElement($dtcacontent);
//  $minteddoi = (string) $sxmldtca->doi;
//  if (empty($minteddoi)) {
//if $minteddoi is empty, then this needs flagging to repomgr
//    $context['results']['pidmissingdoi'][] = $memberpid;
//  }
  $fedora_object = islandora_object_load($memberpid);
  $rels = $fedora_object->relationships;
  $imoc = $rels->get(FEDORA_RELS_EXT_URI, 'isMemberOfCollection');
  $imocname = $imoc[0]['object']['value'];
  $imocprefix = substr($imocname, 0, strpos($imocname, ':'));

  $resource_rdfs = sidora_return_forward_fba_resource_rdfs();
  $concept_rdfs = sidora_return_forward_fba_concept_rdfs();
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
 * 
 * @return array
 */
function _publish_batch_3($topactivitypid, $clonelist) {
  global $user;

  $batch = array(
    'title' => t('First batch run'),
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/batchfunctions.inc',
    'operations' => array(),
    'finished' => '_finished',
  );
  //enumerate through clone list to create new objects
  foreach ($clonelist as $clonepid) {
    if (strcmp($topactivitypid, $clonepid) === 0) {
      array_unshift($batch['operations'], array('cloneobject', array($topactivitypid, TRUE))); //TRUE as it's the root object being submitted
    }
    else {
      array_unshift($batch['operations'], array('cloneobject', array($clonepid, FALSE)));
    }
    $namespace = substr($clonepid, 0, strpos($clonepid, ':'));
    if (strcmp($namespace, 'dtcds') === 0) {
      $pidstomint[] = $clonepid;
    }
  }


  /*
    if ($_SESSION['pidsmissingdois']) {
    $pidsmissingdois = $_SESSION['pidsmissingdois'];
    }
    $operations = array();
    foreach ($pidsmissingdois as $pidmissingdoi) {
    $operations[] = array('_publish_op_3', array($pidmissingdoi));
    }

    $batch = array(
    'operations' => $operations,
    'finished' => '_publish_finished_2',
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/repomgr.publish.inc',
    // We can define custom messages instead of the default ones.
    'title' => t('Processing batch 2'),
    'init_message' => t('Batch 2 is starting.'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('Batch 2 has encountered an error.'),
    );
   * 
   */
  return $batch;
}

function _publish_batch_3_tidy() {
  global $user;

  $batchtidy = array(
    'title' => t('Second batch run'),
    'file' => drupal_get_path('module', 'fba_repo_manager') . '/batchfunctions.inc',
    'operations' => array(),
    'finished' => '_finished',
  );

  // run a second batch to correct all the RELS-EXTs
  $batchresultsuccess = $_SESSION['my_batch_succeeded']; //check return results
  $batchresult = $_SESSION['my_batch_results']; //check return results
//Another final batch job is required here to swap all the old and new pids in the new objects
  $batchtidy['operations'][] = array('tidyuprelations', array($batchresult['success']));

  return $batchtidy;
}

/**
 * Batch operation for batch 3 
 * 
 * @param type $pid
 * @param type $context
 */
function _publish_op_3($pid, &$context) {
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


//TODO: delete this as it's old code.
function createPageThree($pids, array &$form_state) {
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'content_model_viewer', 'Collection');
  //global $base_path;
  //global $base_root;
  global $user;

  $batch = array(
    'title' => t('First batch run'),
    'file' => drupal_get_path('module', 'islandora_content_model_forms') . '/batchfunctions.inc',
    'operations' => array(),
    'finished' => '_finished',
  );

  foreach ($pids as $publishpid) {
    if ((substr($publishpid, 0, strpos($publishpid, ';')) === 'dtcac')) {
      $pid = $publishpid;
      break; //found the activity
    }
  }
  //$user_data = unserialize($user->data);
  //$user_pid = $user_data['islandora_user_pid'];
  //$usernumber = substr($user_pid, strpos($user_pid, ':') + 1); //+1 to ignore the colon
  //Add the relsext here.
  //$selectedpid = $form_state['values']['pidtopublish'];
  $licencepid = $form_state['values']['licencepid'];
  $selectedcollection = $form_state['values']['selectedcollection'];

  $repository = tuque_wrapper_get_repository_instance();
  //$objecttopublish = $repository->getObject($selectedpid);
  //Find which descendants will need publishing
  $childresources = buildChildrenTree($pid, $elements, $user->islandora_user_pid);

  $clonelist = array_merge(array($pid), $childresources); //put the activity at the top, where it should logically appear
  $selectcollectionns = substr($selectedcollection, 0, strpos($selectedcollection, ':'));
  array_unshift($batch['operations'], array('tagobjectforcloning', array($pid, $selectcollectionns, $licencepid, TRUE))); //TRUE as it's the root object being submitted
  //enumerate through clone list to create new objects
  //$remainingrels = $this->list_available_publication_collections($pid);
  //$selectedcollname = $remainingrels[$selectedcollection];

  foreach ($clonelist as $clonepid) {
    array_unshift($batch['operations'], array('tagobjectforcloning', array($clonepid, $selectcollectionns, $licencepid, FALSE)));
    $namespace = substr($clonepid, 0, strpos($clonepid, ':'));
    if (strcmp($namespace, 'dtcds') === 0) {
      $pidstomint[] = $clonepid;
    }
  }
  $resultstring = 'These objects have been marked to be published by Repository Manager: <br/>';
  foreach ($clonelist as $cloneobject) {
    $resultstring .= ' - ' . $cloneobject . '<br/>';
  }
  $resultstring .= 'and these object(s) will be given DOIs: <br/>';
  foreach ($pidstomint as $i => $pidtomint) {
    $resultstring .= ' - ' . $pidtomint . '<br/>';
  }

  batch_set($batch);
  //SFOX bad hack to force the non graphical version of batch processing
  //SFOX as sidora catches the url redirect which shows the percentage complete bar
  $batch = & batch_get();
  $batch['progressive'] = FALSE;
  //end of bad hack
  batch_process();
// run a second batch to correct all the RELS-EXTs
  $batchresultsuccess = $_SESSION['my_batch_succeeded']; //check return results
  $batchresult = $_SESSION['my_batch_results']; //check return results
//Another final batch job is required here to swap all the old and new pids in the new objects
  $batchtidy['operations'][] = array('tidyuprelations', array($batchresult['success']));
  batch_set($batchtidy);
//SFOX bad hack to force the non graphical version of batch processing
//SFOX as sidora catches the url redirect which shows the percent complete bar
  $batchtidy = & batch_get();
  $batchtidy['progressive'] = FALSE;
//end of bad hack
  batch_process();


  $debugstring = '';
  foreach ($batchresult['success'] as $result) {
    $debugstring .= $result[0] . ' -> ' . $result[1] . ' -> ' . $result[2] . '<br/>';
  }
  $redirecturl = $base_root . $base_path . 'fedora/repository/' . $pid;

  $form = array(
    'currentcollections' => array(
      '#type' => 'markup',
      '#value' => 'These are the objects to be cloned/published to ' . $selectedcollection . ': <br/>' . $debugstring,
    ),
    'finish' => array(
      '#type' => 'markup',
      '#value' => '<a href=' . $redirecturl . ' target="_parent">Needs a link here to return to the object screen (the same as if cancel had been clicked)</a>',
    ),
    'pidtopublish' => array(
      '#type' => 'hidden',
      '#value' => $pid,
    ),
    'forms' => array(
      '#type' => 'hidden',
      '#value' => $form_state['values']['forms'],
    ),
  );

  $form = array(
    'currentcollections' => array(
      '#type' => 'markup',
      '#value' => 'These are the objects to be cloned/published to ' . $selectedcollection . ': <br/>' . $resultstring,
    ),
    'pidtopublish' => array(
      '#type' => 'hidden',
      '#value' => $pid,
    ),
    'forms' => array(
      '#type' => 'hidden',
      '#value' => $form_state['values']['forms'],
    ),
  );
  //use this instead?     $form_state['redirect'] = $base_url . '/fedora/repository/' . $pid;
  return $form;
}
