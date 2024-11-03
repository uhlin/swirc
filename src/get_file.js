function get_file(url, name)
{
	var http_req = new ActiveXObject("WinHttp.WinHttpRequest.5.1");
	var stream   = new ActiveXObject("ADODB.Stream");

	http_req.Open("GET", url, false);
	http_req.Send();
	http_req.WaitForResponse();

	stream.Type = 1;
	stream.Open();
	stream.Write(http_req.ResponseBody);
	stream.SaveToFile(name);
}

get_file("https://curl.haxx.se/ca/cacert.pem", "src/trusted_roots.pem");
get_file("https://www.nifty-networks.net/swirc/curl-8.8.0.cab", "curl-8.8.0.cab");
get_file("https://www.nifty-networks.net/swirc/gnu-bundle-202205.cab", "gnu-bundle-202205.cab");
get_file("https://www.nifty-networks.net/swirc/hunspell-1.7.2-dlls.cab", "hunspell-1.7.2.cab");
get_file("https://www.nifty-networks.net/swirc/hunspell-en-us.cab", "hunspell-en-us.cab");
get_file("https://www.nifty-networks.net/swirc/libressl-3.9.2.cab", "libressl-3.9.2.cab");
get_file("https://www.nifty-networks.net/swirc/pdcurses-3.9-utf8-colors.cab", "pdcurses-3.9.cab");
get_file("https://www.nifty-networks.net/swirc/swirc-locales-20241103.cab", "swirc-locales-20241103.cab");
