<?php

class MyClassA {
    public $id = 1;
}

class MyClassB {
    protected $id = "2";
}

class MyClassC {
    private $id = "my_3";
}

class MyClassD {
}

$objectA = new MyClassA();
$objectB = new MyClassB();
$objectC = new MyClassC();
$objectD = new MyClassD();

meminfo_objects_list(fopen('php://stdout', 'w'), "id");
