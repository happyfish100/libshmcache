<?php

$key = 'key_0001';
$value = array('name' => 'yuqing', 'score' => 90.5, 'city' => 'beijing', 'gender' => 'male');
//$value = json_encode($value, JSON_PRETTY_PRINT);

$cache = new ShmCache('/etc/libshmcache.conf', ShmCache::SERIALIZER_IGBINARY);
$cache->set($key, $value, 300);
for ($i=0; $i<10240; $i++) {
    $cache->set($key, $value, 300);
    //$v = $cache->get($key);
}

echo "key $key expires: " . $cache->getExpires($key) . "\n";
var_dump($cache->setExpires($key, time() + 600));
echo "key $key expires: " . $cache->getExpires($key) . "\n";

var_dump($cache->setTTL($key, 900));
echo "key $key expires: " . $cache->getExpires($key) . "\n";

$key1 = 'key_00002';
var_dump($cache->incr($key1, -1, 300));
var_dump($cache->get($key));
var_dump($cache->delete($key));

//var_dump($cache->stats());
echo json_encode($cache->stats(), JSON_PRETTY_PRINT);
//var_dump($cache->clear());
