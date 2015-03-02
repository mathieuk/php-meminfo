<?php

class MyClassA {
    protected $objects = [];

    public function run() {
        $objectD1 = new MyClassD();
        $objectD2 = new MyClassD();
        $objectD3 = new MyClassD();

        $objectD1Bis = $objectD1;

        $objectB1 = new MyClassB();
        $objectB1->setComposedObject1($objectD1);
        $objectB1->setComposedObject2($objectD2);
        $objectB1->composedObject3 = $objectD3;

        $objectC1 = new MyClassC();
        $objectC1->composedObject1 = $objectB1;

        $objectB2 = new MyClassB();
        $objectB2->setComposedObject1($objectD1);
        $objectB1->setComposedObject2($objectD3);

        $i = 125;
        $this->myArray = array(
            'my_object' => $objectD1,
            'my_int' => $i,
            123.5,
            'my_other_string',
            'my_B_object1'=> $objectB1,
            'my_B_object2'=> $objectB2,
            'my_D_objec1bis' => $objectD1Bis
        );
        meminfo_dependency_list(fopen('php://stdout','w'), $this);
    }
}

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

class MyClassC {
    public $composedObject1;
}

class MyClassD {
}

$objectA = new MyClassA();
$objectA->run();



  
