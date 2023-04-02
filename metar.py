import sys, os, requests

url = ('https://www.aviationweather.gov/adds/dataserver_current/httpparam'
        '?dataSource=metars'
        '&requestType=retrieve'
        '&format=xml'
        '&hoursBeforeNow=2'
        '&mostRecentForEachStation=true'
        '&stationString=')
