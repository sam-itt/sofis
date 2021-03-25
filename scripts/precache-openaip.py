#! /usr/bin/python
# SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
#
# This file is part of SoFIS - an open source EFIS
#
# SPDX-License-Identifier: GPL-2.0-only

import argparse
from geolocation import *
import shutil
import math
import subprocess
import os

MIN_LAT=-85.05112878
MAX_LAT=85.05112878
MIN_LON=-180
MAX_LON=180

PROVIDERS=[
    {'url':"https://tile.openstreetmap.org/%d/%d/%d.png",
    'hosts':[]},
    {'url':"https://%s.tile.maps.openaip.net/geowebcache/service/tms/1.0.0/openaip_basemap@EPSG%%3A900913@png/%d/%d/%d.png",
     'hosts':['1','2'],
     'mode':'tms'},
    {'url':"https://%s.tile.maps.openaip.net/geowebcache/service/tms/1.0.0/openaip_approved_airports@EPSG%%3A900913@png/%d/%d/%d.png",
     'hosts':['1','2'],
     'mode':'tms'}
]

STATES={
    'AL':{'name':'Alabama','north':35.008028,'east':-84.889080,'south':30.223334,'west':-88.473227},
    'AK':{'name':'Alaska','north':71.365162,'east':179.778470,'south':51.214183,'west':-179.148909},
    'AS':{'name':'American Samoa','north':-11.046934,'east':-168.143300,'south':-14.548699,'west':-171.089874},
    'AZ':{'name':'Arizona','north':37.004260,'east':-109.045223,'south':31.332177,'west':-114.816510},
    'AR':{'name':'Arkansas','north':36.499600,'east':-89.644395,'south':33.004106,'west':-94.617919},
    'CA':{'name':'California','north':42.009518,'east':-114.131211,'south':32.534156,'west':-124.409591},
    'CO':{'name':'Colorado','north':41.003444,'east':-102.041524,'south':36.992426,'west':-109.060253},
    'MP':{'name':'Commonwealth of the Northern Mariana Islands','north':20.553802,'east':146.064818,'south':14.110472,'west':144.886331},
    'CT':{'name':'Connecticut','north':42.050587,'east':-71.786994,'south':40.980144,'west':-73.727775},
    'DE':{'name':'Delaware','north':39.839007,'east':-75.048939,'south':38.451013,'west':-75.788658},
    'DC':{'name':'District of Columbia','north':38.995110,'east':-76.909395,'south':38.791645,'west':-77.119759},
    'FL':{'name':'Florida','north':31.000888,'east':-80.031362,'south':24.523096,'west':-87.634938},
    'GA':{'name':'Georgia','north':35.000659,'east':-80.839729,'south':30.357851,'west':-85.605165},
    'GU':{'name':'Guam','north':13.654383,'east':144.956712,'south':13.234189,'west':144.618068},
    'HI':{'name':'Hawaii','north':28.402123,'east':-154.806773,'south':18.910361,'west':-178.334698},
    'ID':{'name':'Idaho','north':49.001146,'east':-111.043564,'south':41.988057,'west':-117.243027},
    'IL':{'name':'Illinois','north':42.508481,'east':-87.494756,'south':36.970298,'west':-91.513079},
    'IN':{'name':'Indiana','north':41.760592,'east':-84.784579,'south':37.771742,'west':-88.097760},
    'IA':{'name':'Iowa','north':43.501196,'east':-90.140061,'south':40.375501,'west':-96.639704},
    'KS':{'name':'Kansas','north':40.003162,'east':-94.588413,'south':36.993016,'west':-102.051744},
    'KY':{'name':'Kentucky','north':39.147458,'east':-81.964971,'south':36.497129,'west':-89.571509},
    'LA':{'name':'Louisiana','north':33.019457,'east':-88.817017,'south':28.928609,'west':-94.043147},
    'ME':{'name':'Maine','north':47.459686,'east':-66.949895,'south':42.977764,'west':-71.083924},
    'MD':{'name':'Maryland','north':39.723043,'east':-75.048939,'south':37.911717,'west':-79.487651},
    'MA':{'name':'Massachusetts','north':42.886589,'east':-69.928393,'south':41.237964,'west':-73.508142},
    'MI':{'name':'Michigan','north':48.238800,'east':-82.413474,'south':41.696118,'west':-90.418136},
    'MN':{'name':'Minnesota','north':49.384358,'east':-89.491739,'south':43.499356,'west':-97.239209},
    'MS':{'name':'Mississippi','north':34.996052,'east':-88.097888,'south':30.173943,'west':-91.655009},
    'MO':{'name':'Missouri','north':40.613640,'east':-89.098843,'south':35.995683,'west':-95.774704},
    'MT':{'name':'Montana','north':49.001390,'east':-104.039138,'south':44.358221,'west':-116.050003},
    'NE':{'name':'Nebraska','north':43.001708,'east':-95.308290,'south':39.999998,'west':-104.053514},
    'NV':{'name':'Nevada','north':42.002207,'east':-114.039648,'south':35.001857,'west':-120.005746},
    'NH':{'name':'New Hampshire','north':45.305476,'east':-70.610621,'south':42.696990,'west':-72.557247},
    'NJ':{'name':'New Jersey','north':41.357423,'east':-73.893979,'south':38.928519,'west':-75.559614},
    'NM':{'name':'New Mexico','north':37.000232,'east':-103.001964,'south':31.332301,'west':-109.050173},
    'NY':{'name':'New York','north':45.015850,'east':-71.856214,'south':40.496103,'west':-79.762152},
    'NC':{'name':'North Carolina','north':36.588117,'east':-75.460621,'south':33.842316,'west':-84.321869},
    'ND':{'name':'North Dakota','north':49.000574,'east':-96.554507,'south':45.935054,'west':-104.048900},
    'OH':{'name':'Ohio','north':41.977523,'east':-80.518693,'south':38.403202,'west':-84.820159},
    'OK':{'name':'Oklahoma','north':37.002206,'east':-94.430662,'south':33.615833,'west':-103.002565},
    'OR':{'name':'Oregon','north':46.292035,'east':-116.463504,'south':41.991794,'west':-124.566244},
    'PA':{'name':'Pennsylvania','north':42.269860,'east':-74.689516,'south':39.719800,'west':-80.519891},
    'PR':{'name':'Puerto Rico','north':18.515683,'east':-65.220703,'south':17.883280,'west':-67.945404},
    'RI':{'name':'Rhode Island','north':42.018798,'east':-71.120570,'south':41.146339,'west':-71.862772},
    'SC':{'name':'South Carolina','north':35.215402,'east':-78.542030,'south':32.034600,'west':-83.353910},
    'SD':{'name':'South Dakota','north':45.945450,'east':-96.436589,'south':42.479635,'west':-104.057698},
    'TN':{'name':'Tennessee','north':36.678118,'east':-81.646900,'south':34.982972,'west':-90.310298},
    'TX':{'name':'Texas','north':36.500704,'east':-93.508292,'south':25.837377,'west':-106.645646},
    'VI':{'name':'United States Virgin Islands','north':18.412655,'east':-64.564907,'south':17.673976,'west':-65.085452},
    'UT':{'name':'Utah','north':42.001567,'east':-109.041058,'south':36.997968,'west':-114.052962},
    'VT':{'name':'Vermont','north':45.016659,'east':-71.464555,'south':42.726853,'west':-73.437740},
    'VA':{'name':'Virginia','north':39.466012,'east':-75.242266,'south':36.540738,'west':-83.675395},
    'WA':{'name':'Washington','north':49.002494,'east':-116.915989,'south':45.543541,'west':-124.763068},
    'WV':{'name':'West Virginia','north':40.638801,'east':-77.719519,'south':37.201483,'west':-82.644739},
    'WI':{'name':'Wisconsin','north':47.080621,'east':-86.805415,'south':42.491983,'west':-92.888114},
    'WY':{'name':'Wyoming','north':45.005904,'east':-104.052160,'south':40.994746,'west':-111.056888}
}

COUNTRIES={
    'AU':{'name':'Australia','north':-9.088228,'east':168.224954,'south':-55.322817,'west':72.246094},
    'AT':{'name':'Austria','north':49.020530,'east':17.160776,'south':46.372276,'west':9.530749},
    'BE':{'name':'Belgium','north':51.551667,'east':6.408097,'south':49.496982,'west':2.388914},
    'CH':{'name':'Switzerland','north':47.808465,'east':10.492294,'south':45.817995,'west':5.955911},
    'DE':{'name':'Germany','north':55.099161,'east':15.041932,'south':47.270111,'west':5.866315},
    'DK':{'name':'Denmark','north':57.952430,'east':15.553064,'south':54.451667,'west':7.715325},
    'ES':{'name':'Spain','north':43.993309,'east':4.591888,'south':27.433543,'west':-18.393684},
    'FR':{'name':'France','north':51.268318,'east':9.867834,'south':41.263219,'west':-5.453429},
    'GR':{'name':'Greece','north':41.748886,'east':29.729699,'south':34.700610,'west':19.247788},
    'IT':{'name':'Italy','north':47.092146,'east':18.784475,'south':35.288962,'west':6.627266},
    'LU':{'name':'Luxembourg','north':50.430377,'east':6.034425,'south':49.496982,'west':4.968441},
    'PT':{'name':'Portugal','north':42.154311,'east':-6.189159,'south':29.828802,'west':-31.557530},
    'CA':{'name':'Canada','north':83.336213,'east':-52.323198,'south':41.676556,'west':-141.002750},
    'GB':{'name':'United-Kingdom','north':25.224165,'east':55.160692,'south':25.222319,'west':55.157952}
}

def check_dependencies():
    rv = True
    if shutil.which('curl') is None:
        print("Missing dependency: %scurl%s use your system package manager to install it" %(
            '\033[1;31m', #start bold red
            '\033[0m' #reset
        ))
        rv = False
    if shutil.which('composite') is None:
        print("Missing dependency: %sImageMagick%s (%scomposite%s binary) use your system package manager to install it" %(
            '\033[1;31m', #start bold red
            '\033[0m', #reset
            '\033[1;31m', #start bold red
            '\033[0m' #reset

        ))
        rv = False
    return rv

def clamp(value, low_bound, high_bound):
    return min(max(value, low_bound), high_bound)

def geo_to_tile(latitude, longitude, level):
    latitude = clamp(latitude, MIN_LAT, MAX_LAT)
    longitude = clamp(longitude, MIN_LON, MAX_LON)

    x = (longitude + 180.0) / 360.0
    sin_lat = math.sin(latitude * math.pi/180.0)
    y = 0.5 - math.log((1+sin_lat)/(1-sin_lat)) / (4*math.pi)

    mapsize = 256 * 1 << level
    #Pixel coordinates
    world_x = clamp(x*mapsize + 0.5, 0, mapsize-1)
    world_y = clamp(y*mapsize + 0.5, 0, mapsize-1)

    tile_x = world_x // 256
    tile_y = world_y // 256

    return tile_x, tile_y

def download_tile_area(dest_dir,level, top, left, bottom, right, recover=False):
    current_host = [0] * len(PROVIDERS)

    total=((bottom-top)+1)*((right-left)+1)
    done=int(0)
    print("Doing level %d: 0%%\r" % (level),end='',flush=True)
    for tilex in range(int(left),int(right)+1):
        for(tiley) in range(int(top), int(bottom)+1):
            outfile='%s/%d/%d/%d.png'%(dest_dir,level,tilex,tiley)
            if recover and os.path.exists(outfile):
                continue
            for i in range(len(PROVIDERS)):
                final_tiley = tiley
                if 'mode' in PROVIDERS[i] and PROVIDERS[i]['mode'] == 'tms':
                    # openaip layers use TMS, 0,0 is bottom left
                    # instead of top left, so we need to reverse y
                    tms_maxy = (1 << level) - 1
                    tmsy = tms_maxy - tiley
                    #print("TMS converstion y: %d -> %d" % (tiley,tmsy))
                    final_tiley = tmsy
                if 'hosts' in PROVIDERS[i] and len(PROVIDERS[i]['hosts']) > 0:
                    turl = PROVIDERS[i]['url'] % (
                        PROVIDERS[i]['hosts'][current_host[i]],
                        level, tilex, final_tiley
                    )
                    current_host[i] += 1
                    if current_host[i] == len(PROVIDERS[i]['hosts']):
                        current_host[i] = 0
                else:
                    turl = PROVIDERS[i]['url'] % (level, tilex, final_tiley)
#                print("url: %s" % turl)
                output = "%d.png" %i
                subprocess.run(["curl", turl, "-o",output],
                    stderr=subprocess.DEVNULL,
                    stdout=subprocess.DEVNULL
                )
            for i in range(1,len(PROVIDERS) - 1):
                layer = "%d.png" %i
                if not os.path.exists(layer):
                    continue
                subprocess.run(["composite", layer, "0.png", "0.png"])
                os.unlink(layer)
            os.makedirs('%s/%d/%d/' % (dest_dir,level,tilex),exist_ok=True)
            os.rename("0.png", outfile)
            done += 1
            print("Doing level %d: %0.2f%%\r" % (level, done/total*100.0),end='',flush=True)
    print() #prints a \n


parser = argparse.ArgumentParser()
subparsers = parser.add_subparsers(help='sub-command help',
    dest='command',
    required=True
)

# Global arguments
parser.add_argument('-l','--min-level', type=int,
    help='Start grabbing tiles at given level (default 6)',
    default=6
)
parser.add_argument('-L','--max-level', type=int,
    help='Stop grabbing tiles at given level (default 12)',
    default=12
)
parser.add_argument('-d','--dest-dir', type=str,
    help='Where to put the files (default current directory)',
    default='.'
)
parser.add_argument('-r','--resume', action='store_true',
    help='Do not override existing tiles',
)

# Area subcommand
area_parser = subparsers.add_parser('area', help='Fetch an area based on lat/lon and radius')
area_parser.add_argument("latitude", type=float, help="latitude of the area center")
area_parser.add_argument("longitude", type=float, help="longitude of the area center")
area_parser.add_argument('-r','--radius', type=int,
    help='Set the radius in nautical miles (default 6)',
    default=6
)

# State subcommand
state_parser = subparsers.add_parser('state', help='Fetch tiles for a given state (TX/FL/etc.)',)
state_parser.add_argument("state_code", type=str, help="Code of the state to fetch")

# State subcommand
country_parser = subparsers.add_parser('country', help='Fetch tiles for a given country (FR/UK/DE/etc.)',)
country_parser.add_argument("country_code", type=str, help="Code of the country")


args = parser.parse_args()
#Check deps before doing anything
if not check_dependencies():
    exit(-1)


north=0
east=0
south=0
west=0
if args.command == 'state':
    if args.state_code not in STATES:
        print("%s: Unknown code state, bailing out" % args.state_code)
        exit(-1)
    print("Fetching levels %d-%d for %s - %s" % (
        args.min_level,args.max_level,
        args.state_code, STATES[args.state_code]['name']
    ))
    north = STATES[args.state_code]['north']
    east = STATES[args.state_code]['east']
    south = STATES[args.state_code]['south']
    west = STATES[args.state_code]['west']
elif args.command == 'country':
    if args.country_code not in COUNTRIES:
        print("%s: Unknown code state, bailing out" % args.country_code)
        exit(-1)
    print("Fetching levels %d-%d for %s - %s" % (
        args.min_level,args.max_level,
        args.country_code, COUNTRIES[args.country_code]['name']
    ))
    north = COUNTRIES[args.country_code]['north']
    east = COUNTRIES[args.country_code]['east']
    south = COUNTRIES[args.country_code]['south']
    west = COUNTRIES[args.country_code]['west']
elif args.command == 'area':
    print('Fetching %d km around center lat:%f lon:%f min-level:%d max-level:%d' % (
        args.radius, args.latitude,
        args.longitude,args.min_level, args.max_level
    ))
    loc = GeoLocation.from_degrees(args.latitude, args.longitude)
    distance = args.radius*1.852  # N.M to kilometers
    SW_loc, NE_loc = loc.bounding_locations(distance)

    north = NE_loc.deg_lat
    east = NE_loc.deg_lon
    south = SW_loc.deg_lat
    west = SW_loc.deg_lon


print("Fetching area:\nup left - up right: (%f,%f) - (%f,%f)\ndown left - down right: (%f,%f) - (%f,%f)" % (
        north, west,
        north, east,

        south, west,
        south, east,
    )
)


for level in range(args.min_level, args.max_level+1):
    right, top = geo_to_tile(north, east, level)
    left, bottom = geo_to_tile(south, west, level)

    #print("area in tiles - level %d: top:%d left:%d bottom: %d right:%d" % (level, top, left, bottom, right))

    download_tile_area(args.dest_dir,level, top, left, bottom, right, args.resume)
