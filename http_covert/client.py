from urlparse import urlparse
import requests , os , re

checked_urls = set()

# ripped from LinkFinder
def send_request(url):
    '''
    Send requests with Requests
    '''
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) \
        AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36',
        'Accept': 'text/html,\
        application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
        'Accept-Language': 'en-US,en;q=0.8',
        'Accept-Encoding': 'gzip'
    }

    content = requests.get(url, headers=headers, timeout=1, stream=True, verify=False)
    return content

# ripped from LinkFinder
def spider(url,path) :
	regex = re.compile(r"""
		(%s(?:"|')                    # Start newline delimiter
		(?:
		((?:[a-zA-Z]{1,10}://|//)       # Match a scheme [a-Z]*1-10 or //
		[^"'/]{1,}\.                    # Match a domainname (any character + dot)
		[a-zA-Z]{2,}[^"']{0,})          # The domainextension and/or path
		|
		((?:/|\.\./|\./)                # Start with /,../,./
		[^"'><,;| *()(%%$^/\\\[\]]       # Next character can't be... 
		[^"'><,;|()]{1,})               # Rest of the characters can't be
		|
		([a-zA-Z0-9_\-/]{1,}/           # Relative endpoint with /
		[a-zA-Z0-9_\-/]{1,}\.[a-z]{1,4} # Rest + extension
		(?:[\?|/][^"|']{0,}|))          # ? mark with parameters
		|
		([a-zA-Z0-9_\-]{1,}             # filename
		\.(?:php|asp|aspx|jsp)          # . + extension
		(?:\?[^"|']{0,}|))              # ? mark with parameters

		)             

		(?:"|')%s)                    # End newline delimiter
		""" % ("",""), re.VERBOSE)

	s = send_request(url+"/"+path)

	if s.status_code != 200 :
		return 

	if s.headers['Content-Type'] != "text/html" : 
		return 

	items = re.findall(regex, s.text)
	items = list(set(items))
	for l in items :
		group = list(filter(None, l))[1]
		parsed = urlparse(group)
		if parsed.netloc not in url :
			break
		if group not in checked_urls :
			checked_urls.add(group)
			spider(url,group)

def main() :
	URL = "http://localhost:8080"	
	PATH  = ""
	spider(URL,PATH)

	for l in checked_urls :
		print l

	#while(1) :
	#	c = input('$ ')

if __name__ == '__main__':
	main()
