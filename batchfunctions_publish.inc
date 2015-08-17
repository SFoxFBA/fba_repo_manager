<?php

//function cloneobject($pid, $rootobject, &$context) {
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
              swaprelsvalues(RELS_EXT_URI, $allrdf, $batchresults, $object);
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
              $x = 1;
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
    //if it's a dataset, then phase 5 needs to know about it
    if (strcmp(substr($privatepid, 0, strpos($privatepid, ':')), 'dtcds') === 0) {
      $context['results']['success'][] = array($privatepid, $publicpid, TRUE);
    }
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
    return array($foundrelpid, $foundrelreturnpid);
  }
  else {
    return NULL;
  }
}

function downloadablefilesizes($publicpid, &$context) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  $cumulativesize = $cumulativenumoffiles = $datacompnumoffiles = $datacompdssize = 0;
  $results = get_all_core_data_components($publicpid);
  foreach ($results as $resultpid) {
    $datacomp_object = islandora_object_load($resultpid);
    $dtca = $datacomp_object['DTCA'];
    $dtcacontent = $dtca->content;
    $dtcaxml = simplexml_load_string($dtcacontent);
    $metadataonlytext = (string) $dtcaxml->publishMetadataOnly;

    if (strcmp($metadataonlytext, 'No') === 0) {
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
    }

    if ($datacompnumoffiles === 0) {
      $datacompdssize = 'na';
    }
    $datacomp_object->relationships->add('http://purl.org/dc/terms/', 'extent', $datacompdssize, RELS_TYPE_PLAIN_LITERAL);
    $datacompdssize = 0;
    $datacomp_object->relationships->add(FBA_URI, 'hasDownloadableFiles', $datacompnumoffiles, RELS_TYPE_PLAIN_LITERAL);
    $datacompnumoffiles = 0;
  }
  //now add the cumulative totals to the dataset
  $rootobject = islandora_object_load($publicpid);
  if ($cumulativenumoffiles === 0) {
    $cumulativesize = 'na';
  }
  $rootobject->relationships->add('http://purl.org/dc/terms/', 'extent', $cumulativesize, RELS_TYPE_PLAIN_LITERAL);
  $rootobject->relationships->add(FBA_URI, 'hasDownloadableFiles', $cumulativenumoffiles, RELS_TYPE_PLAIN_LITERAL);
}

function get_all_core_data_components($pid) {
  module_load_include('inc', 'sidora', 'includes/fba');
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');

  $predicates = array_merge(sidora_return_fba_resource_rdfs(), sidora_return_fba_other_rdfs());
  $object = islandora_object_load($pid);

  foreach ($predicates as $predicate) {
    $rels = $object->relationships->get('info:fedora/fedora-system:def/relations-external#', $predicate);
    if (count($rels) > 0) {
      foreach ($rels as $rel) {
        $relsresults[] = $rel['object']['value'];
      }
    }
  }
  return $relsresults;
}