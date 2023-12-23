#!/usr/bin/python3
'''
Upload local files (e.g. RPi installation packages) as assets to a GitHub release.
It authorizes with the repo token and gets the upload_url for the relevant tag.
Existing remote files with the same name will be deleted before uploading.
Defaults to: OpenHantek/OpenHantek6022/releases/tags/devdrop
'''

#########################
# Default repo settings #
#########################
#                       #
OWNER = 'OpenHantek'    #
REPO = 'OpenHantek6022' #
#                       #
#########################


import sys
import os.path
import requests
from http.client import responses
import json
import mimetypes
import argparse


# construct the argument parser and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument( "-o", "--owner", default = OWNER,
	help = f"specify the repository owner, default = '{OWNER}'" )
ap.add_argument( "-r", "--repo", default = REPO,
	help = f"specify the repository, default = '{REPO}'" )
ap.add_argument( "-t", "--tag", default = 'devdrop',
	help = f"specify a tag, default = 'devdrop'" )
ap.add_argument( "-l", "--latest", action='store_true',
	help = f"select 'latest' release, overide '-t' or '--tag'" )
ap.add_argument( "-d", "--devdrop", action='store_true',
	help = f"select 'devdrop' release, overide '-t' or '-l'" )
ap.add_argument( 'files', metavar='FILE', type=str, nargs='*',
	help = f"file to be uploaded" )

options = ap.parse_args()

OWNER = options.owner
REPO = options.repo
TAG = options.tag

if options.latest:
	TAG = 'latest'

if options.devdrop:
	TAG = 'devdrop'

files = options.files

#print( OWNER, REPO, TAG, files )

###########################
# github api v3 settings: #
# developer.github.com/v3 #
###########################
#
API = 'https://api.github.com/repos/'


################################
# get my personal access token #
# (github.com/settings/tokens) #
################################
#
TOKENFILE = os.path.expanduser( '~/.config/GitHub/REPO_TOKEN' )
with open( TOKENFILE, 'r' ) as tokenfile:
	TOKEN = tokenfile.readline().strip()
	if len( TOKEN ) != 40:
		print( "Authorization token not found" )
		sys.exit()


headers = { 'Authorization': 'token ' + TOKEN, }


#####################################
# build the right URL for this repo #
#####################################
#
MYREPO = OWNER + '/' + REPO


#######################
# check authorization #
#######################
#
timeout = ( 10, 30 ) # (connect, response) seconds time out
url = API + MYREPO
response = requests.get( url, headers=headers, timeout=timeout )
#print( response.status_code, url )
if response.status_code != 200:
	print( 'Authorizing error', response.status_code, responses[response.status_code], url )
	sys.exit( -1 )


##################################
# build the releases part of URL #
##################################
#
RELEASES = MYREPO + '/releases/'


#####################
# get the right tag #
#####################
#
if TAG and TAG != 'latest':
	TAG = 'tags/' + TAG


###################################
# get the upload URL for this tag #
###################################
#
url = API + RELEASES + TAG
response = requests.get( url, headers=headers, timeout=timeout )
#print( response.status_code, url )
if response.status_code != 200:
	print( 'Communication error', response.status_code, responses[response.status_code], url )
	sys.exit( -1 )
rj = response.json()


# UPLOADURL is an URI with a format like this:
# https://uploads.github.com/repos/OpenHantek/OpenHantek6022/releases/12345678/assets{?name,label}
# truncate URI at 1st '{', because the asset name will be applied via "param = {'name' : NAME}"
UPLOADURL = rj[ 'upload_url' ].rsplit('{')[0]

# uncomment to pretty print the response
#print( json.dumps( rj, indent=4 ) )

assets = rj['assets']
# check if we have any files to display
if not assets:
	sys.exit() # we are done


# get (and show) already uploaded assets
print( f"{(RELEASES + TAG).ljust(60)}              updated   dl" )
already_uploaded_assets = {}
for a in assets:
	name = a['name']
	url = a['url']
	already_uploaded_assets[ name ] = url
	print( f"{name.ljust(60)} {a['updated_at']} {a['download_count']:#4}" )


# check if we have any files to upload
if not files:
	sys.exit() # we are done


############################
# try to upload all assets #
############################
#

print('Uploading ...')

processed = 0
success = 0

for ASSET in files:
	processed += 1

	print( ASSET, flush=True, end='' )

	if not os.path.isfile( ASSET ):
		print( ' - file not found' )
		continue

	NAME = os.path.basename( ASSET ) # remove the leading path

	# if an asset with same name already exists, delete its URL
	if NAME in already_uploaded_assets.keys():
		print( ' - delete remote file', flush=True, end='' )
		try:	# delete existing asset (URL)
			response = requests.delete( already_uploaded_assets[ NAME ], headers=headers, timeout=timeout )
			if not response:
				print( ' error', response.status_code, responses[response.status_code] )
		except Exception as ex:
			print( ' error', ex )

	# prepare the mimetype header entry accordig to the file extension (typically '.deb', '.rpm' or '.tgz')
	MIMETYPE, app = mimetypes.guess_type( NAME )
	if MIMETYPE:
		headers[ 'Content-Type' ] = MIMETYPE
	else:
		headers[ 'Content-Type' ] = 'application/octet-stream'

	# define parameter 'name'
	params = { 'name' : NAME }

	try:
		with open( ASSET, 'rb' ) as asset:
			print( ' - upload', flush=True, end = '' )
			try:
				response = requests.post( UPLOADURL, headers=headers, params=params, data=asset, timeout=timeout )
				if not response:
					print( ' - upload error', response.status_code, responses[response.status_code] )
					continue
			except Exception as ex:
				print( ' - upload error', ex )
				continue
			else:
				print( )
				success += 1
	except Exception as ex:
		print( ' - file error', ex )
		continue

if processed:
	print( success, 'of', processed, 'file(s) successfully uploaded' )
