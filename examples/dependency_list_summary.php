<?php

class MyClassA {
    public $objects;

    public $object1;
    public $object2;
    public $object3;
}

class MyClassB {
    public $links;
}

class MyClassC {
}

class MyClassD {
}

$objectA = new MyClassA();
$objectB = new MyClassB();

$objectA->objects = [
    "B1" => $objectB,
    "B2" => $objectB, // TODO: same instance, should be considered as count = 1 !
    "A1" => new MyClassA(),
    "A2" => new MyClassA(),
    "A3" => new MyClassA(),
    "A4" => new MyClassA()
];

$array1 = [
    "D1" => new MyClassD(),
    "D2" => new MyClassD(),
    "D3" => new MyClassD(),
    "D4" => new MyClassD()
];

$array2 = ["my_array_1" => $array1 ];
$array3 = ["my_array_2" => $array2 ];

$objectB->links = [$array3];

$objectA->object1 = new MyClassC();
$objectA->object2 = new MyClassD();
$objectA->object3 = new MyClassA();

meminfo_dependency_list(fopen('php://stdout','w'), $objectA);
meminfo_dependency_list_summary(fopen('php://stdout','w'), $objectA);

