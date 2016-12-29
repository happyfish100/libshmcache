<?php

$key = 'key_0001';
$value = array('name' => 'yuqing', 'score' => 90.5, 'city' => 'beijing', 'gender' => 'male');

$host = '10.0.11.89';
$port = 6379;
$connectTimeout = 1;
$redis = new \Redis();
$redis->connect($host, $port, $connectTimeout);
$redis->setOption(\Redis::OPT_SERIALIZER, \Redis::SERIALIZER_IGBINARY);
var_dump($redis->set($key, $value, 300));
for ($i=0; $i<10000; $i++) {
    //$redis->set($key, $value, 300);
    $v = $redis->get($key);
}
var_dump($redis->get($key));
var_dump($redis->delete($key));

