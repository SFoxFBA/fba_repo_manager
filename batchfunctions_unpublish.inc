<?php

/**
 * Used as part of the repository manager's dataset publication batch jobs.
 */
function _rdfremove($leftpid, $ns, $rdf, $rightpid, &$context) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');
  $object = islandora_object_load($leftpid);

  $checkrelsext = $object->relationships->get($ns, $rdf, NULL, RELS_TYPE_PLAIN_LITERAL);
  $countcheckrelsext = count($checkrelsext);
  if ($countcheckrelsext > 0) {
    for ($i = 0; $i < $countcheckrelsext; $i++) {
      $retval[$rightpid] = $object->relationships->remove($ns, $rdf, NULL, RELS_TYPE_PLAIN_LITERAL);
    }
  }
  $context['results']['rdfremove'][$leftpid] = $retval;
  return;
}

function _resetdtca($pid, &$context) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');

  $object = islandora_object_load($pid);

  $dtca = $object['DTCA'];
  $dtcacontent = $dtca->content;
  if ($dtcacontent) {
    $sxmldtca = new SimpleXMLElement($dtcacontent);
    unset($sxmldtca->archivalPublicationState);
    unset($sxmldtca->archivalPublicationLicencePid);
    unset($sxmldtca->archivalPublicationCollection);
    unset($sxmldtca->archivalPublicationVersion);
    unset($sxmldtca->archivalPublicationDate);
    $object['DTCA']->content = $sxmldtca->asXML();
    $context['results']['resetdtca'][$pid] = 'complete';
  }
  else {
    $context['results']['resetdtca'][$pid] = 'dtcacontent not populated';
  }

  return;
}

function _purgeobject($pid, &$context) {
  module_load_include('module', 'tuque_wrapper', 'tuque_wrapper');

  $repository = tuque_wrapper_get_repository_instance();

  $retval = $repository->purgeObject($pid);
  $context['results']['purgeobject'][$pid] = $retval; //true or false

  return;
}
