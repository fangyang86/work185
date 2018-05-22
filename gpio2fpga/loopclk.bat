#!/bin/sh
c=0;
until [ $c -eq 3 ]
do
  echo 1 > clk
  echo 0 > clk
done

