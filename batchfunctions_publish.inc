<?php

/**
 * Used as part of the repository manager's dataset publication batch jobs.
 * This one extracts the observed property values, and writes them to the DTCA
 * datastream. Created as part of JIRA:AEDA-22
 * 
 * @param type $pid     (unique identifier of object to be processed)
 * @param type $context (used by the drupal batch mechanism)
 */
function updatedtca($pid, &$context) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/fbasidora/common');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item'); //for the FBA_MOLES_URI constant

  $fedora_object = islandora_object_load($pid);
  $dtcadatastream = $fedora_object['DTCA'];
  $dtcadscontent = $dtcadatastream->content;
  $domdtca = DOMDocument::loadXML($dtcadscontent);
  $datacomp = $domdtca->getElementsByTagName('DataComponent')->item(0);
  //get the current vocabterm list from the DTCA datastream
  $observedProperties = $domdtca->getElementsByTagName('observedProperty');
  if ($observedProperties->length > 0) {
    foreach ($observedProperties as $observedProperty) {
      $characterStrings = $domdtca->getElementsByTagName('CharacterString');
      foreach ($characterStrings as $characterString) {
        if ($characterString->nodeValue) {
          $dtcatermstrings[] = trim($characterString->nodeValue);
        }
      }
      break;
    }
  }
  if (!isset($dtcatermstrings)) {
    $dtcatermstrings = array();
  }


//get the current label list from the DTCA datastream
  $labelparameters = $domdtca->getElementsByTagName('parameter');
  if ($labelparameters->length > 0) {
    foreach ($labelparameters as $labelparameter) {
      $namedvalues = $labelparameter->getElementsByTagName('NamedValue');
      foreach ($namedvalues as $namedvalue) {
        $dtcalabelname[] = trim($namedvalue->nodeValue);
      }
    }
    $uniquedtcalabelname = array_unique($dtcalabelname);
  }
  else {
    $uniquedtcalabelname = array();
  }

  $repository = tuque_wrapper_get_repository_instance();
  //build a list that needs adding from all the attached csvs

  $repo_object = $repository->getObject($pid);
  foreach ($repo_object as $datastream) {
    $dsid = $datastream->id;
    $dsidprefix = substr($dsid, 0, 3);
    if (strcmp($dsidprefix, 'OBJ') === 0) {
      $dsids[] = $datastream->id;
    }
  }

  //get the OBJ datastreams and extract the labels and potentially new observed 
  //properties from it
  foreach ($dsids as $dsid) {
    $objdatastream = $repository->api->a->getDatastreamDissemination($repo_object->id, $dsid, NULL); // Work around for bug in Tuque API: ISLANDORA-752
    // Processing First Row of CSV file (user defined headers)
    // Extract to an array containing each label/header
    $csvheaders = trim_array_values(explode(",", strtok($objdatastream, "\n")));
    if (!is_null($uniquecsvlabels)) { //cannot array merge to null array
      $uniquecsvlabels = array_merge($uniquecsvlabels, array_unique($csvheaders));
    }
    else {
      $uniquecsvlabels = array_unique($csvheaders);
    }

    // Processing Second Row of CSV files (vocab terms)
    // Extract to an array containing each vocab_term then convert to DTC vocab pid
    $vocab_terms = explode(",", strtok("\n"));
    $terms = array_map('trim', $vocab_terms);
    if (!is_null($uniquecsvterms)) {//cannot array merge to null array
      $uniquecsvterms = array_merge($uniquecsvterms, $terms);
    }
    else {
      $uniquecsvterms = $terms; //taken off the unique function for now
    }
  }
  $uniquecsvlabels = array_unique($uniquecsvlabels);
  $uniquecsvterms = array_unique($uniquecsvterms);

  //compare to create add/delete list
  $termstoadd = compare_arrays_for_addordelete($uniquecsvterms, $dtcatermstrings, FALSE); //FALSE for add
  $termstoremove = compare_arrays_for_addordelete($uniquecsvterms, $dtcatermstrings, TRUE); //TRUE for remove

  $labelstoadd = compare_arrays_for_addordelete($uniquecsvlabels, $uniquedtcalabelname, FALSE); //FALSE for add
  $labelstoremove = compare_arrays_for_addordelete($uniquecsvlabels, $uniquedtcalabelname, TRUE); //TRUE for remove
  $checkrel = $repo_object->relationships->get(FBA_URI, 'hasObservedProperty');
  if (count($checkrel) > 0) {
    foreach ($checkrel as $rel) {
      $relsresults[] = $rel['object']['value'];
    }
  }
  if (!isset($relsresults)) {
    $relsresults = array();
  }

  //add in the new header values.
  if (count($termstoadd) > 0) {
    //add in the new DTCA observed properties.
    foreach ($termstoadd as $termtoadd) {
      $obspropxml = $domdtca->createElementNS('http://fba.org.uk/schemas/moles/3.4.0', 'observedProperty');
      $charstrxml = $domdtca->createElement('gco:CharacterString', $termtoadd);
      $obspropxml->appendChild($charstrxml);
      $datacomp->appendChild($obspropxml);
      //check if there's a rels ext for this to the vocab term, if not, add it.
      $vocab_terms_ns_pid = (string) _check_vocab_term($termtoadd);
      $vocab_term_pid = substr($vocab_terms_ns_pid, strpos($vocab_terms_ns_pid, '/') + 1);
      if (count($relsresults) > 0) {
        if (!in_array($vocab_term_pid, $relsresults)) {
          module_load_include('inc', 'fedora_repository', 'api/fedora_item');
          $item = new Fedora_Item($pid);
          $item->add_relationship('hasObservedProperty', $vocab_term_pid, FBA_URI);
        }
      }
      else {
        //there are no existing relsexts for this so just add it
        module_load_include('inc', 'fedora_repository', 'api/fedora_item');
        $item = new Fedora_Item($pid);
        $item->add_relationship('hasObservedProperty', $vocab_term_pid, FBA_URI);
      }
    }
  }
  //add in the DTCA labels
  if (count($labelstoadd) > 0) {
    $parameterxml = $domdtca->createElementNS('http://fba.org.uk/schemas/moles/3.4.0', 'parameter');
    foreach ($labelstoadd as $labeltoadd) {
      $namedvalxml = $domdtca->createElementNS('http://fba.org.uk/schemas/moles/3.4.0', 'NamedValue');
      $namexml = $domdtca->createElementNS('http://fba.org.uk/schemas/moles/3.4.0', 'name', $labeltoadd);
      $namedvalxml->appendChild($namexml);
      $parameterxml->appendChild($namedvalxml);
      $datacomp->appendChild($parameterxml);
    }
  }

  if (count($termstoremove) > 0) {
    foreach ($termstoremove as $termtoremove) {
      foreach ($observedProperties as $observedProperty) {
        $obspropnv = trim($observedProperty->nodeValue);
        if (strcmp($termtoremove, $obspropnv) === 0) {
          if ($observedProperty->parentNode) {
            $observedProperty->parentNode->removeChild($observedProperty);
            break; //found one so no need to search for another, duplicates are allowed
          }
        }
      }
      //check if there's a rels ext for this to the vocab term, if so, remove it.
      $vocab_terms_ns_pid = (string) _check_vocab_term($termtoremove);
      $vocab_term_pid = substr($vocab_terms_ns_pid, strpos($vocab_terms_ns_pid, '/') + 1);
      if (!is_null($relsresults)) {
        if (in_array($vocab_term_pid, $relsresults)) {
          module_load_include('inc', 'fedora_repository', 'api/fedora_item');
          $item = new Fedora_Item($pid);
          $item->purge_relationships('hasObservedProperty', $vocab_term_pid, FBA_URI);
        }
      }
    }
  }

  //Also clear out any labels (mo:parameter/mo:NamedValue/mo:name) 
  if (count($labelstoremove) > 0) {
    foreach ($labelstoremove as $labeltoremove) {
      foreach ($labelparameters as $labelparameter) {
        $labparnv = trim($labelparameter->nodeValue);
        if (strcmp($labeltoremove, $labparnv) === 0) {
          if ($labelparameter->parentNode) {
            $labelparameter->parentNode->removeChild($labelparameter);
          }
          break; //found one so no need to search for another, duplicates are allowed
        }
      }
    }
  }

  $fedora_object['DTCA']->content = $domdtca->saveXML(); //write the datastream xml back to the object
}

/**
 * 
 * @param array $firstarray
 * @param array $secondarray
 * @param boolean $addordeleteflag FALSE for add, TRUE for delete
 */
function compare_arrays_for_addordelete($firstarray, $secondarray, $addordeleteflag) {
  $arraytoreturn = array();
  if (count($firstarray) > 0) {
    asort($firstarray);
  }
  if (count($secondarray) > 0) {
    asort($secondarray);
  }
  switch ($addordeleteflag) {
    case FALSE: {
        //work out what needs adding
        if ((count($firstarray) > 0) && (count($secondarray) > 0)) {
          foreach ($secondarray as $secondarrayvalue) {
            $key = array_search($secondarrayvalue, $firstarray);
            if ($key !== FALSE) {
              unset($firstarray[$key]);
            }
          }
        }
        $arraytoreturn = $firstarray;
        break;
      }
    case TRUE: {
        //work out what needs deleting
        if ((count($firstarray) > 0) && (count($secondarray) > 0)) {
          foreach ($firstarray as $firstarrayvalue) {
            $key = array_search($firstarrayvalue, $secondarray);
            if ($key !== FALSE) {
              unset($secondarray[$key]);
            }
          }
        }
        $arraytoreturn = $secondarray;
        break;
      }
  }
  return $arraytoreturn;
}

function cloneobject($pid, &$context) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/utils');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $connection = islandora_get_tuque_connection();
  $repository = $connection->repository;

  //get the selectedcollection and licensepid from the DTCA datastream
  $policydsid = 'POLICY';
  $fedora_object = islandora_object_load($pid);
  $clonedobject = islandora_copy_object($fedora_object);

  //Need to ignore the POLICY datastream during cloning.
  $clonedobject->purgeDatastream($policydsid);

  //Get the collection and license pid from the DTCA datastream
  $sourcedtca = $fedora_object['DTCA'];
  $sourcedtcacontent = $sourcedtca->content;
  $sourcedtcads = simplexml_load_string($sourcedtcacontent);

  $selectedcollection = (string) $sourcedtcads->archivalPublicationCollection;
  $licencepid = (string) $sourcedtcads->archivalPublicationLicencePid;

  //need to change the objectId (pid) of the cloned object
  $newpid = $repository->getNextIdentifier($selectedcollection);
  $clonedobject->id = $newpid;

  //Tidy up the datastreams
  $dtca = $clonedobject['DTCA'];
  $dtcacontent = $dtca->content;
  $dtcads = simplexml_load_string($dtcacontent);
  $dtcads->archivalPublicationVersion++;
  sidora_update_datastream($clonedobject, $dtcads->asXML(), 'DTCA', NULL, 'text/xml');
  $metadataonlytext = (string) $dtcads->publishMetadataOnly;

  //Remove the restrictive POLICY datastream, and replace with an open one,
  //dependent on the Metadata only flag.
  if (strcmp($metadataonlytext, 'Yes') === 0) {
    sidora_update_datastream($clonedobject, create_published_xacml(), $policydsid, NULL, 'text/xml');
  }
  else {
    sidora_update_datastream($clonedobject, create_open_xacml(), $policydsid, NULL, 'text/xml');
  }

  $rels = $clonedobject->relationships;
  $rels->add(FBA_URI, 'hasLicence', $licencepid);

  $ingestobject = islandora_add_object($clonedobject);
  if ($ingestobject) {
    //if the ingest works then
    $ingestsuccessful = TRUE;
  }
  else {
    //if the ingest fails then
    $ingestsuccessful = FALSE;
  }
  $context['results']['success'][] = array($pid, $newpid, $ingestsuccessful);

  return;
}

function create_published_xacml(stdClass $account = NULL) {
  module_load_include('inc', 'islandora_xacml_api', 'Xacml');
  global $user;
  $account = is_object($account) ? $account : $user;
  $xacml = new Xacml();
  $xacml->managementRule->addUser(array('fedoraAdmin', $account->name));
  $xacml->managementRule->addRole(array('administrator'));
  //Removed next two lines, so all users can view unrestricted datastreams
  //$xacml->viewingRule->addUser(array('fedoraAdmin', $account->name));
  //$xacml->viewingRule->addRole(array('administrator'));
  $xacml->datastreamRule->addDsid('RELS-INT');
  $xacml->datastreamRule->addDsid('TN');
  $xacml->datastreamRule->addDsid('EXIF');
  $xacml->datastreamRule->addDsid('MEDIUM_SIZE');
  $xacml->datastreamRule->addDsid('TECHMD_FITS');
  $xacml->datastreamRule->addDsidRegex('^OBJ');
  $xacml->datastreamRule->addDsidRegex('^COLS');
  $xacml->datastreamRule->addUser(array('fedoraAdmin', $account->name));
  $xacml->datastreamRule->addRole(array('administrator'));

  $xml = $xacml->getXmlString(TRUE);
  return $xml;
}

function create_open_xacml(stdClass $account = NULL) {
  module_load_include('inc', 'islandora_xacml_api', 'Xacml');
  global $user;
  $account = is_object($account) ? $account : $user;
  $xacml = new Xacml();
  $xacml->managementRule->addUser(array('fedoraAdmin', $account->name));
  $xacml->managementRule->addRole(array('administrator'));
  //$xacml->viewingRule->addUser(array('fedoraAdmin', $account->name));
  //$xacml->viewingRule->addRole(array('administrator'));
  $xml = $xacml->getXmlString(TRUE);
  return $xml;
}

function tidyuprelations($batchresults, &$context) {
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/utils');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  foreach ($batchresults as $batchresult) {
    $privatepid = $batchresult[0];
    $publicpid = $batchresult[1];
    $object = islandora_object_load($publicpid);
    $dtca = $object['DTCA'];
    $dtcacontent = $dtca->content;
    $dtcads = simplexml_load_string($dtcacontent);
    $selectedcollectionns = (string) $dtcads->archivalPublicationCollection;

    $lastchainpid = findlastpidinchain($privatepid, FBA_URI, 'hasPublishedVersion');

    foreach ($object as $dsid => $notused) {
      switch ($dsid) {
        case 'DC': {
            //Apply the new publish transformation 
            $transform = drupal_get_path('module', 'islandora_content_model_forms') . "/transforms/dtca_to_dc_publish.xsl";
            $xsl = new DOMDocument();
            $xsl->load($transform);
            $xslt = new XSLTProcessor();
            $xslt->importStyleSheet($xsl);
            $ds = DOMDocument::loadXML($dtcacontent);
            $transformeddoc = $xslt->transformToDoc($ds);
            $topleveldc = $transformeddoc->getElementsByTagNameNS('http://www.openarchives.org/OAI/2.0/oai_dc/', 'dc')->item(0);
            $dcid = $transformeddoc->createElement('dc:identifier', $publicpid);
            $topleveldc->appendChild($dcid);
            $object['DC']->content = $transformeddoc->saveXML();
            break;
          }
        case 'RELS-EXT': {
            //isMemberOfCollection fbauser<xxx>:collection needs swapping to dtc<xxxx>
            $rels = $object->relationships;
            $imocs = $rels->get(FEDORA_RELS_EXT_URI, 'isMemberOfCollection');
            foreach ($imocs as $imoc) {
              $imocvalue = $imoc[object][value];
              //$user_id = $user->islandora_user_pid;
              //$user_collection_ns = str_replace(array('-', ':'), '', $user_id);
              //$user_collection = $user_collection_ns . ':collection';
              //if (strcmp($imocvalue, $user_collection) === 0) {
              $rels->remove(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $imocvalue);
              $selectedcollectionns = (string) $dtcads->archivalPublicationCollection;
              $rels->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $selectedcollectionns . ':collection');
              //}
            }
            $rels->remove(FEDORA_MODEL_URI, 'hasModel', 'si:conceptCModel');
            $rels->remove(FEDORA_MODEL_URI, 'hasModel', 'si:resourceCModel');
            //isComponentInputOf dtcan:xx needs swapping to the published (eg) analysis pid
            swaprelsvalues(FBA_URI, 'isComponentInputOf', $batchresults, $object);
            //URL RDFS's dtcan:xx needs swapping to the published (eg) analysis pid
            $allurlrdfs = sidora_return_fba_url_rdfs();
            foreach ($allurlrdfs as $allurlrdf) {
              swaprelsvalues(FBA_URI, $allurlrdf, $batchresults, $object);
            }

            //Handle hasXXXX relsext here
            $allrdfs = array_merge(sidora_return_fba_concept_rdfs(), sidora_return_fba_resource_rdfs());
            foreach ($allrdfs as $allrdf) {
              swaprelsvalues(FBA_URI, $allrdf, $batchresults, $object);
            }

            //As full versioning code isn't in place, will just force this for now
            $rels->add(FBA_URI, 'isLatestVersion', 'yes', RELS_TYPE_PLAIN_LITERAL);

            break;
          }
        case 'DTCA': {
            //the new objects archival version number needs setting to 1 greater 
            //than current highest
            $sxmldtca = new SimpleXMLElement($dtcacontent);
            $nowdate = sprintf("%sT%s.000Z", date("Y-m-d"), date("H:i:s"));
            $sxmldtca->archivalPublicationDate = $nowdate;
            //Version number needs to be taken from last published in the chain
            if (is_null($lastchainpid)) {
              //$sxmldtca->archivalPublicationVersion++;
            }
            else {
              $lastchainobject = islandora_object_load($lastchainpid[0]);
              $lastchaindtca = $lastchainobject['DTCA'];
              $lastchaindtcacontent = $lastchaindtca->content;
              $lastchainsxmldtca = new SimpleXMLElement($lastchaindtcacontent);
              $lastchainversion = (int) $lastchainsxmldtca->archivalPublicationVersion;
              //$sxmldtca->archivalPublicationVersion = $lastchainversion + 1;
            }

            unset($sxmldtca->archivalPublicationState);
            unset($sxmldtca->archivalPublicationLicencePid);
            unset($sxmldtca->archivalPublicationCollection);

            $object['DTCA']->content = $sxmldtca->asXML();
            break;
          }
        case 'RELS-INT': {
            //change the rdf:description/rdf:about value.
            $ri = $object['RELS-INT'];
            $fri = new IslandoraFedoraRelsInt($ri);
            $fri->changeObjectID($publicpid);

            //change the hasConstituent pid value, remembering that it has a suffix of (eg) /COLS-1-1
            $ricontent = $ri->content;
            $newricontent = str_replace($privatepid, $publicpid, $ricontent);
            $object['RELS-INT']->content = $newricontent;
            break;
          }
        default: {
            if (strpos($dsid, 'COLS') !== FALSE) { //Could also be DTCA, POLICY, OBJ<n>
              //COLSx-x datastream
              //<rdf:description rdf:about> needs pid swapping
              //rdf:description/dc:identifier needs pid swapping out, noting that there's a suffix 
              $ri = $object[$dsid];
              $ricontent = $ri->content;
              $newricontent = str_replace($privatepid, $publicpid, $ricontent);
              $object[$dsid]->content = $newricontent;
            }
          }
      }
    }
    //Need to add the hasPublishedVersion relsext now, to the last object in the chain
    if (is_null($lastchainpid)) {
      $pidtoupdate = $privatepid;
      $pidtopointto = $publicpid;
    }
    else {
      $pidtoupdate = $lastchainpid[0];
      $pidtopointto = $publicpid;
    }

    $privateobject = islandora_object_load($pidtoupdate);
    $privrels = $privateobject->relationships;
    $privrels->add(FBA_URI, 'hasPublishedVersion', $pidtopointto);

    $dtcapriv = $privateobject['DTCA'];
    $dtcacontentpriv = $dtcapriv->content;
    $sxmldtca = new SimpleXMLElement($dtcacontentpriv);
    unset($sxmldtca->archivalPublicationState);
    unset($sxmldtca->archivalPublicationLicencePid);
    unset($sxmldtca->archivalPublicationCollection);
    $privateobject['DTCA']->content = $sxmldtca->asXML();
    //if it's a dataset or datacomponent, then phase 5 needs to know about it
    if (sidora_is_datacomponent($privatepid) || sidora_is_dataset($privatepid)) {
      $context['results']['success'][] = array($privatepid, $publicpid, TRUE);
    }
  }
}

function tidyuprelation($batchresults, $relationpid, &$context) {
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/utils');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $privatepid = $relationpid[0];
  $publicpid = $relationpid[1];
  $object = islandora_object_load($publicpid);
  $dtca = $object['DTCA'];
  $dtcacontent = $dtca->content;
  $dtcads = simplexml_load_string($dtcacontent);
  $selectedcollectionns = (string) $dtcads->archivalPublicationCollection;

  $lastchainpid = findlastpidinchain($privatepid, FBA_URI, 'hasPublishedVersion');

  foreach ($object as $dsid => $notused) {
    switch ($dsid) {
      case 'DC': {
          //Apply the new publish transformation 
          $transform = drupal_get_path('module', 'islandora_content_model_forms') . "/transforms/dtca_to_dc_publish.xsl";
          $xsl = new DOMDocument();
          $xsl->load($transform);
          $xslt = new XSLTProcessor();
          $xslt->importStyleSheet($xsl);
          $ds = DOMDocument::loadXML($dtcacontent);
          $transformeddoc = $xslt->transformToDoc($ds);
          $topleveldc = $transformeddoc->getElementsByTagNameNS('http://www.openarchives.org/OAI/2.0/oai_dc/', 'dc')->item(0);
          $dcid = $transformeddoc->createElement('dc:identifier', $publicpid);
          $topleveldc->appendChild($dcid);
          $object['DC']->content = $transformeddoc->saveXML();
          break;
        }
      case 'RELS-EXT': {
          //isMemberOfCollection fbauser<xxx>:collection needs swapping to dtc<xxxx>
          $rels = $object->relationships;
          $imocs = $rels->get(FEDORA_RELS_EXT_URI, 'isMemberOfCollection');
          foreach ($imocs as $imoc) {
            $imocvalue = $imoc[object][value];
            //$user_id = $user->islandora_user_pid;
            //$user_collection_ns = str_replace(array('-', ':'), '', $user_id);
            //$user_collection = $user_collection_ns . ':collection';
            //if (strcmp($imocvalue, $user_collection) === 0) {
            $rels->remove(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $imocvalue);
            $selectedcollectionns = (string) $dtcads->archivalPublicationCollection;
            $rels->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $selectedcollectionns . ':collection');
            //}
          }
          $rels->remove(FEDORA_MODEL_URI, 'hasModel', 'si:conceptCModel');
          $rels->remove(FEDORA_MODEL_URI, 'hasModel', 'si:resourceCModel');
          //isComponentInputOf dtcan:xx needs swapping to the published (eg) analysis pid
          swaprelsvalues(FBA_URI, 'isComponentInputOf', $batchresults, $object);
          //URL RDFS's dtcan:xx needs swapping to the published (eg) analysis pid
          $allurlrdfs = sidora_return_fba_url_rdfs();
          foreach ($allurlrdfs as $allurlrdf) {
            swaprelsvalues(FBA_URI, $allurlrdf, $batchresults, $object);
          }

          //Handle hasXXXX relsext here
          $allrdfs = array_merge(sidora_return_fba_concept_rdfs(), sidora_return_fba_resource_rdfs());
          foreach ($allrdfs as $allrdf) {
            swaprelsvalues(FBA_URI, $allrdf, $batchresults, $object);
          }

          //As full versioning code isn't in place, will just force this for now
          $rels->add(FBA_URI, 'isLatestVersion', 'yes', RELS_TYPE_PLAIN_LITERAL);

          break;
        }
      case 'DTCA': {
          //the new objects archival version number needs setting to 1 greater 
          //than current highest
          $sxmldtca = new SimpleXMLElement($dtcacontent);
          $nowdate = sprintf("%sT%s.000Z", date("Y-m-d"), date("H:i:s"));
          $sxmldtca->archivalPublicationDate = $nowdate;
          //Version number needs to be taken from last published in the chain
          if (is_null($lastchainpid)) {
            //$sxmldtca->archivalPublicationVersion++;
          }
          else {
            $lastchainobject = islandora_object_load($lastchainpid[0]);
            $lastchaindtca = $lastchainobject['DTCA'];
            $lastchaindtcacontent = $lastchaindtca->content;
            $lastchainsxmldtca = new SimpleXMLElement($lastchaindtcacontent);
            $lastchainversion = (int) $lastchainsxmldtca->archivalPublicationVersion;
            //$sxmldtca->archivalPublicationVersion = $lastchainversion + 1;
          }

          unset($sxmldtca->archivalPublicationState);
          unset($sxmldtca->archivalPublicationLicencePid);
          unset($sxmldtca->archivalPublicationCollection);

          $object['DTCA']->content = $sxmldtca->asXML();
          break;
        }
      case 'RELS-INT': {
          //change the rdf:description/rdf:about value.
          $ri = $object['RELS-INT'];
          $fri = new IslandoraFedoraRelsInt($ri);
          $fri->changeObjectID($publicpid);

          //change the hasConstituent pid value, remembering that it has a suffix of (eg) /COLS-1-1
          $ricontent = $ri->content;
          $newricontent = str_replace($privatepid, $publicpid, $ricontent);
          $object['RELS-INT']->content = $newricontent;
          break;
        }
      default: {
          if (strpos($dsid, 'COLS') !== FALSE) { //Could also be DTCA, POLICY, OBJ<n>
            //COLSx-x datastream
            //<rdf:description rdf:about> needs pid swapping
            //rdf:description/dc:identifier needs pid swapping out, noting that there's a suffix 
            $ri = $object[$dsid];
            $ricontent = $ri->content;
            $newricontent = str_replace($privatepid, $publicpid, $ricontent);
            $object[$dsid]->content = $newricontent;
          }
        }
    }
  }
  //Need to add the hasPublishedVersion relsext now, to the last object in the chain
  if (is_null($lastchainpid)) {
    $pidtoupdate = $privatepid;
    $pidtopointto = $publicpid;
  }
  else {
    $pidtoupdate = $lastchainpid[0];
    $pidtopointto = $publicpid;
  }

  $privateobject = islandora_object_load($pidtoupdate);
  $privrels = $privateobject->relationships;
  $privrels->add(FBA_URI, 'hasPublishedVersion', $pidtopointto);

  $dtcapriv = $privateobject['DTCA'];
  $dtcacontentpriv = $dtcapriv->content;
  $sxmldtca = new SimpleXMLElement($dtcacontentpriv);
  unset($sxmldtca->archivalPublicationState);
  unset($sxmldtca->archivalPublicationLicencePid);
  unset($sxmldtca->archivalPublicationCollection);
  $privateobject['DTCA']->content = $sxmldtca->asXML();
  //if it's a dataset or datacomponent, then phase 5 needs to know about it
  if (sidora_is_datacomponent($privatepid) || sidora_is_dataset($privatepid)) {
    $context['results']['success'][] = array($privatepid, $publicpid, TRUE);
  }
}

function swaprelsvalues($namespace, $relation, $batchresults, &$object) {
  $rels = $object->relationships;
  $relations = $rels->get($namespace, $relation);
  foreach ($relations as $relvalue) {
    $relobject = $relvalue[object];
    $objvalue = $relobject[value];
    foreach ($batchresults as $translationpair) {
      if (strcmp($objvalue, $translationpair[0]) === 0) {
        //Can swap this over as there's a match
        $rels->remove($namespace, $relation, $objvalue);
        $rels->add($namespace, $relation, $translationpair[1]);
      }
    }
  }
}

function findlastpidinchain($pid, $namespace, $relation) {
  $object = islandora_object_load($pid);
  $rels = $object->relationships;
  $foundrelations = $rels->get($namespace, $relation);
  if ($foundrelations) {
    $foundrelpid = $foundrelations[0][object][value];
    $foundrelreturnpid = findlastpidinchain($foundrelpid, $namespace, $relation);
    $returnval = array($foundrelpid, $foundrelreturnpid);
  }
  else {
    $returnval = NULL;
  }
  return $returnval;
}

function downloadablefilesizes($privatepid, $publicpid, &$context) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  module_load_include('inc', 'sidora', 'includes/fba');

  $cumulativesize = $cumulativenumoffiles = $datacompnumoffiles = $datacompdssize = 0;
  $results = get_all_core_data_components($publicpid);
  foreach ($results as $resultpid) {
    $datacomp_object = islandora_object_load($resultpid);

    //get the filesize
    foreach ($datacomp_object as $datastream) {
      if (preg_match('/^OBJ/', $datastream->id)) {
        $datacompdssize+= $datastream->size; //in bytes
        $datacompnumoffiles++;
      }
    }
    $cumulativenumoffiles = $cumulativenumoffiles + $datacompnumoffiles;
    if (is_numeric($datacompdssize)) {
      $cumulativesize+= intval($datacompdssize);
    }

    if ($datacompnumoffiles === 0) {
      $datacompdssize = 'na';
    }
    //Quick sanity check to ensure that no values exist already
    $checkrelextent = $datacomp_object->relationships->get('http://purl.org/dc/terms/', 'extent', NULL, RELS_TYPE_PLAIN_LITERAL);
    $countcheckrelext = count($checkrelextent);
    if ($countcheckrelext > 0) {
      for ($i = 0; $i < $countcheckrelext; $i++) {
        $datacomp_object->relationships->remove('http://purl.org/dc/terms/', 'extent', NULL, RELS_TYPE_PLAIN_LITERAL);
      }
    }
    $datacomp_object->relationships->add('http://purl.org/dc/terms/', 'extent', $datacompdssize, RELS_TYPE_PLAIN_LITERAL);
    $datacompdssize = 0; //reset for next loop round

    $checkrelhdf = $datacomp_object->relationships->get(FBA_URI, 'hasDownloadableFiles', NULL, RELS_TYPE_PLAIN_LITERAL);
    $countcheckrelhdf = count($checkrelhdf);
    if ($countcheckrelhdf > 0) {
      for ($i = 0; $i < $countcheckrelhdf; $i++) {
        $datacomp_object->relationships->add(FBA_URI, 'hasDownloadableFiles', NULL, RELS_TYPE_PLAIN_LITERAL);
      }
    }
    $datacomp_object->relationships->add(FBA_URI, 'hasDownloadableFiles', $datacompnumoffiles, RELS_TYPE_PLAIN_LITERAL);
    $datacompnumoffiles = 0; //reset for next loop round
  }
  //now add the cumulative totals to the dataset
  $rootobject = islandora_object_load($publicpid);
  if ($cumulativenumoffiles === 0) {
    $cumulativesize = 'na';
  }

  $checkrootext = $rootobject->relationships->get('http://purl.org/dc/terms/', 'extent', NULL, RELS_TYPE_PLAIN_LITERAL);
  $countrootrelext = count($checkrootext);
  if ($countrootrelext > 0) {
    for ($i = 0; $i < $countrootrelext; $i++) {
      $rootobject->relationships->remove('http://purl.org/dc/terms/', 'extent', $cumulativesize, RELS_TYPE_PLAIN_LITERAL);
    }
  }
  $rootobject->relationships->add('http://purl.org/dc/terms/', 'extent', $cumulativesize, RELS_TYPE_PLAIN_LITERAL);

  $checkroothdf = $rootobject->relationships->get(FBA_URI, 'hasDownloadableFiles', NULL, RELS_TYPE_PLAIN_LITERAL);
  $countrootrelhdf = count($checkroothdf);
  if ($countrootrelhdf > 0) {
    for ($i = 0; $i < $countrootrelhdf; $i++) {
      $rootobject->relationships->remove(FBA_URI, 'hasDownloadableFiles', NULL, RELS_TYPE_PLAIN_LITERAL);
    }
  }
  $rootobject->relationships->add(FBA_URI, 'hasDownloadableFiles', $cumulativenumoffiles, RELS_TYPE_PLAIN_LITERAL);
}

function get_all_core_data_components($pid) {
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');

  $predicates = array_merge(sidora_return_fba_resource_rdfs(), sidora_return_fba_other_rdfs());
  $object = islandora_object_load($pid);

  foreach ($predicates as $predicate) {
    $rels = $object->relationships->get(FBA_URI, $predicate);
    if (count($rels) > 0) {
      foreach ($rels as $rel) {
        $relsresults[] = $rel['object']['value'];
      }
    }
  }
  return $relsresults;
}
