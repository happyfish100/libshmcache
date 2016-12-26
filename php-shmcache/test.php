<?php

$key = 'key_0001';
$value = 'value1234';
$cache = new ShmCache('/etc/libshmcache.conf');
$cache->set($key, $value, 300);
var_dump($cache->get($key));
var_dump($cache->delete($key));

//var_dump($cache->stats());
echo json_encode($cache->stats(), JSON_PRETTY_PRINT);
