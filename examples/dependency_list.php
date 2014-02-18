<?php

class MyClassA {};

class MyClassB {
    private $composedObject1;
    protected $composedObject2;
    public $composedObject3;

    public function setComposedObject1($object)
    {
        $this->composedObject1 = $object;
    }

    public function setComposedObject2($object)
    {
        $this->composedObject2 = $object;
    }
}

$o = new MyClassA();
$o2 = new MyClassA();
$o3 = new MyClassA();

$obis = $o;

$ob = new MyClassB();
$ob->composedObject3 = $o3;
$ob->setComposedObject2($o2);
$ob->setComposedObject1($ob);

$i = 125;
$a = array(
    'my_object' => $o,
    'my_int' => $i,
    123.5,
    'my_other_string',
    'my_B_object'=> $ob,
    'my_object_bis' => $obis
);

$GLOBALS['my_array'] = $a;

meminfo_dependency_list(fopen('php://stdout','w'), $GLOBALS);


  
