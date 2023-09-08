#!/usr/bin/env python3

import argparse
from datetime import datetime as dt
import json
import pandas as pd
import plotly.express as px
import sys

ap = argparse.ArgumentParser()
ap.add_argument('-i', action="store", dest="inp", metavar="inputfile")
ap.add_argument('-t', action="store", dest="title", metavar="title",
                default=f"No Title Given ({dt.ctime(dt.now())})")
ap.add_argument('-o', action="store", dest="out", metavar="outputfile")
ap.add_argument('-z', action="store", dest="zoom", metavar="zoom_level", default=15)

args = ap.parse_args()

data = {"utc":{},
        "rcvr":{},
        "lat":{}, "lon":{}, "ele":{},
        "rssi":{}, "snr":{},
        "sndr":{}
        }

def parse_peers_data(f):
    global data
    n = 0
    while True:
        s = f.readline()
        if len(s) == 0:
            break
        # print(s.strip())
        s = s.split(' ')
        if len(s) == 1:
            continue
        i = s.index("[R") if s[1] == "A" else s.index("[Q")
        part1 = s[:i]   # our (receiver) details
        part2 = s[i+1:] # sender's details

        # we parse our part of any received message
        t = s[0]
        if part1[-1][:4] == "gps=": # only consider data if we had valid gps
            # print(part1[-1])
            gps = part1[-1][4:].split(',')
            data["utc"][n]  = part1[0]
            data["rcvr"][n] = part1[2]
            data["lat"][n] = gps[0]
            data["lon"][n] = gps[1]
            data["ele"][n] = gps[2]
            data["rssi"][n] = int(part1[4][5:-3])
            data["snr"][n]  = float(part1[5][4:])
            data["sndr"][n] = part2[0]
            n += 1
        # else:
        #     gps = [0,0,0]

        # A received message is either a "pong" (A) or a "ping" (R):
        #      in case of pong: extract sender's details (= what _they_ received)
        if s[1] == 'A': 
            i = part2.index("[Q")
            part3 = part2[i+1:]  # details from whom they received stuff
            part2 = part2[:i]    # their details
            if part2[-1][:4] == "gps=": # only consider if they had valid gps
                gps = part2[-1][4:].split(',')
                data["utc"][n]  = t
                data["rcvr"][n] = part2[0]
                data["lat"][n] = gps[0]
                data["lon"][n] = gps[1]
                data["ele"][n] = gps[2]
                data["rssi"][n] = int(part2[2][5:-3])
                data["snr"][n]  = float(part2[3][4:])
                data["sndr"][n] = part3[0]
                n += 1
            # else:
            #     gps = [0,0,0]
    

if args.inp == None:
    parse_peers_data(sys.stdin)
else:
    with open(args.inp, 'r') as f:
        parse_peers_data(f)
            
df = pd.read_json(json.dumps(data))
# print(df)

fig = px.scatter_mapbox(df, lat='lat', lon='lon',
                        hover_data=['utc','rcvr','rssi','snr','sndr'],
                        color='rssi',
                        range_color=[-121,-40],
                        # color_continuous_scale=px.colors.cyclical.IceFire,
                        # size_max=15,
                        # center=dict(lat=47.548, lon=7.61),
                        mapbox_style='stamen-terrain',
                        zoom=args.zoom,
                        title=args.title)

if args.out == None:
    fig.show()
else:
    # fig.write_image(args.out, width=2048, height=1536)
    fig.write_image(args.out, width=1024, height=768)
