#!/usr/bin/python
#
# Graph IDMEF Messages
#

import time
import sys
sys.path.append('.')
sys.path.append('./.libs')

import gd

try:
       import PreludeEasy
except:
       print "Import failed"
       print "Try 'cd ./.libs && ln -s libprelude_python.so _PreludeEasy.so'"
       sys.exit(1)

#
# GD Constants
#

timeline_x = 100
severity_x = 300
classification_x = 500

header_size_y = 20
image_width = 800
image_height = 400+header_size_y

severity_high_y = 50 + header_size_y
severity_medium_y = 150 + header_size_y
severity_low_y = 250 + header_size_y
severity_info_y = 350 + header_size_y

im = gd.image((image_width, image_height))

white = im.colorAllocate((255, 255, 255))
black = im.colorAllocate((0, 0, 0))
red = im.colorAllocate((255, 0, 0))
orange = im.colorAllocate((255, 100, 0))
blue = im.colorAllocate((0, 0, 255))
green = im.colorAllocate((0, 255, 0))


client = PreludeEasy.Client("PoolingTest")
client.Init()

client.PoolInit("192.168.33.215", 1)

def gd_init():
       FONT = "/usr/share/fonts/truetype/ttf-bitstream-vera/VeraMono.ttf"

       # Headers
       im.line((0,header_size_y),(image_width,header_size_y),black)
       im.string_ttf(FONT, 8, 0, (70,12), "timeline", black)
       im.line((200,0),(200,header_size_y), black)
       im.string_ttf(FONT, 8, 0, (250,12), "impact.severity", black)
       im.line((400,0),(400,header_size_y), black)
       im.string_ttf(FONT, 8, 0, (450,12), "classification.text", black)
       im.line((600,0),(600,header_size_y), black)

       # Line for timeline
       im.line((timeline_x,header_size_y),(timeline_x,image_height),black)
       # Lines for severity
       im.line((severity_x,header_size_y),(severity_x,image_height-300),red)
       im.line((severity_x,image_height-300),(severity_x,image_height-200),orange)
       im.line((severity_x,image_height-200),(severity_x,image_height-100),green)
       im.line((severity_x,image_height-100),(severity_x,image_height),blue)
       # Line for classification.text
       im.line((classification_x,header_size_y),(classification_x,image_height),black)

#        return im

gd_init()

def plot_timeline():
       t = time.localtime()
       hour = t[3]
       minute = t[4]
       second = t[5]

       hour_factor = 400.0 / 24.0
       mn_factor = hour_factor / 60.0

       hour_y = hour_factor * hour
       mn_y = mn_factor * minute

       plot_y = hour_y + mn_y

       return int(plot_y)


#
# 10000 could be considered as the maximum, since
# it would cover already a big classification.text
#
def unique_alert_number(ClassificationText):
       number = 0

       for c in ClassificationText:
              number += ord(c)

       return number


def classification_text_pos(text):
       classification_factor = 400.0 / 10000.0
       nb = unique_alert_number(text)
       print "Unique number = " + str(nb)

       c_y = classification_factor * nb
       print "Position C-Y = " + str(c_y)


       return int(c_y + header_size_y)


def handle_alert(idmef):      

       classificationtext = idmef.Get("alert.classification.text")
       print classificationtext
#        if value:
#               print value

#        value = idmef.Get("alert.assessment.impact.description")
#        if value:
#               print value

#        value = idmef.Get("alert.assessment.impact.completion")
#        if value:
#               print value
              
#        value = idmef.Get("alert.classification.ident")
#        if value:
#               print value

#        value = idmef.Get("alert.source(0).ident")
#        if value:
#               print value

#        value = idmef.Get("alert.classification.ident")
#        if value:
#               print value

#        value = idmef.Get("alert.classification.reference(0).origin")
#        if value:
#               print value

#        value = idmef.Get("alert.classification.reference(0).name")
#        if value:
#               print value

       severity = idmef.Get("alert.assessment.impact.severity")
       if severity:
              time_y = plot_timeline() + header_size_y
              print "Time Y = " + str(time_y)
              if severity == "high":
                     im.line((timeline_x, time_y),(severity_x, severity_high_y), black)
                     if classificationtext:
                            c_y = classification_text_pos(classificationtext)
                            im.line((severity_x, severity_high_y),(classification_x, c_y), black)
              if severity == "medium":
                     im.line((timeline_x, time_y),(severity_x, severity_medium_y), black)
                     if classificationtext:
                            c_y = classification_text_pos(classificationtext)
                            im.line((severity_x, severity_medium_y),(classification_x, c_y), black)
              if severity == "low":
                     im.line((timeline_x, time_y),(severity_x, severity_low_y), black)
                     if classificationtext:
                            c_y = classification_text_pos(classificationtext)
                            im.line((severity_x, severity_low_y),(classification_x, c_y), black)
              if severity == "info":
                     im.line((timeline_x, time_y),(severity_x, severity_info_y), black)
                     if classificationtext:
                            c_y = classification_text_pos(classificationtext)
                            im.line((severity_x, severity_info_y),(classification_x, c_y), black)

#        print "hour=" + str(hour) + " mn=" + str(minute)



       im.writePng("idmef-graph.png") 


while 1:
       idmef = client.ReadIDMEF(1)
       if idmef:
              handle_alert(idmef)

       time.sleep(1)

