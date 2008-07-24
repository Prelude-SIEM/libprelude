#!/usr/bin/python

import PreludeEasy

client = PreludeEasy.ClientEasy("PoolingTest", PreludeEasy.Client.IDMEF_READ)
client.Start()

while True:
    idmef = PreludeEasy.IDMEF()

    ret = client.RecvIDMEF(idmef)
    if ret:
	print idmef
