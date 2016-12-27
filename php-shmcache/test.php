<?php

echo "start1\n";
system('ps auxww | grep test.php');

$key = 'key_0001';
$value = array('name' => 'yuqing', 'score' => 90.5, 'city' => 'beijing', 'gender' => 'male');
//$value = json_encode($value, JSON_PRETTY_PRINT);

$cache = new ShmCache('/etc/libshmcache.conf', ShmCache::SERIALIZER_IGBINARY);

gc_collect_cycles();
echo "start2\n";
system('ps auxww | grep test.php');

for ($i=0; $i<1024; $i++) {
    $cache->set($key, $value, 300);
}
var_dump($cache->get($key));
var_dump($cache->delete($key));

sleep(1);
gc_collect_cycles();
system('ps auxww | grep test.php');
echo "done\n";

//var_dump($cache->stats());
echo json_encode($cache->stats(), JSON_PRETTY_PRINT);
