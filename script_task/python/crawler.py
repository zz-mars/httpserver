#!/usr/bin/python
#coding=utf-8
# grantchen 2014/08/24

import urllib2
import re
import sys
import string

def isabsurl(url):
	if any(url.startswith(x) for x in ('https://www.', 'http://www.', 'https://', 'http://', 'ftp://', 'file://')):
		return True
	return False

# add prefix to a url if needed
def urladdprefix(url):
	if isabsurl(url):
		return url
	if url.startswith('www'):
		url = 'http://' + url
	else:
		url = 'http://www.' + url
	return url

# crawl a single url
# write the crawled stuff into file
# return a list of urls in this url page
def urlcrawler(url, fcrawl_file):
	print "crawl url -> %s\n" % (url)
	try:
		response = urllib2.urlopen(url, timeout=5)
		the_page = response.read()
		# write to file
		fcrawl_file.write(the_page)
	except:
		print "cannot open -> %s\n" % url
		return []
	# get the urls in the page
	href_pattern = re.compile(r'<a.*href="([^"]*)"')
	match = href_pattern.findall(the_page)
	print "inside links -> ", match
	return match

	
def geturlrootbase(url):
	pattern_str = r'(^.*://[^/]*/?)'
	pattern = re.compile(pattern_str)
	match = pattern.match(url)
	if match:
		return match.group(0)
	return url

def geturldotbase(url):
	pattern_str = r'(^.*://.*/)'
	pattern = re.compile(pattern_str)
	match = pattern.match(url)
	if match:
		return match.group(0)
	return url

def getabsurl(base_url, url):
	if url.startswith('/'):
		# root_dir + /xxx
		url = geturlrootbase(base_url) + url
	elif url.startswith('./'):
		# current_dir + /xxx
		url = base_url + url[1:]
	elif url.startswith('../'):
		# parent_dir + /xxx
		url = geturldotbase(base_url.strip('/')) + url[2:]
	else:
		url = base_url + "/" + url
	return url


def urlscrawler(dict_urls, crawl_file):
	# open file for write
	try:
		fcrawl_file = open(crawl_file, "w+")
	except IOError:
		print "open file error -> %s\n" % (crawl_file)
		return {}

	next_dict_urls = {}
	for base_url in dict_urls:
		urls_list = dict_urls[base_url]
		for url in urls_list:
			# skip tags
			if url.startswith('#'):
				continue
			if not isabsurl(url):
				# get abs url if needed
				url = getabsurl(base_url, url)
			# add prefix if needed
			url = urladdprefix(url)
			next_base = geturldotbase(url)
			next_dict_urls[next_base] = urlcrawler(url, fcrawl_file)
	fcrawl_file.close()
	return next_dict_urls

if __name__ == '__main__':
	if len(sys.argv) != 3:
		print "Usage : %s [url] [crawl_depth]\n" % (sys.argv[0])
		sys.exit(1)
	origin_url = sys.argv[1]
	crawl_depth = sys.argv[2]
	try:
		crawl_depth = int(crawl_depth)
	except ValueError:
		sys.exit(1)
	if crawl_depth <= 0:
		print "invalid crawl_depth -> %d\n" (crawl_depth)
		sys.exit(1)

	dict_urls = {}
	# original url with base set to ''
	urls_list = []
	origin_url = urladdprefix(origin_url)
	urls_list.append(origin_url)
	dict_urls[''] = urls_list
	for depth in xrange(crawl_depth):
		print dict_urls
		crawl_depth_file = "crawl_file_" + str(depth)
		print "crawl_depth -> %d\n" % (depth)
		# crawl the urls
		dict_urls = urlscrawler(dict_urls, crawl_depth_file)

