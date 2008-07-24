#!/usr/bin/python

import sys
import PreludeEasy

idmef = PreludeEasy.IDMEF()
idmef.Set("alert.classification.text", "Bar")

client = PreludeEasy.ClientEasy("MyTest")
client << idmef

