/*
 * Downloads multiple Windows specific build files.
 * Written by Markus Uhlin.
 */

function append_file(target, source)
{
	var ForReading = 1;
	var ForAppending = 8;

	try {
		var fso = new ActiveXObject("Scripting.FileSystemObject");

		var targetFile = fso.OpenTextFile(target, ForAppending);
		var sourceFile = fso.OpenTextFile(source, ForReading);

		var content = sourceFile.ReadAll();
		sourceFile.Close();

		targetFile.Write(content);
		targetFile.Close();

		WScript.Echo("append_file: succeeded!");
	} catch (ex) {
		WScript.Echo("append_file: " + ex.message);
	}
}

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

get_file("https://www.nifty-networks.net/swirc/letsencrypt.pem", "src/letsencrypt.pem");
get_file("https://curl.haxx.se/ca/cacert.pem", "src/trusted_roots.pem");
append_file("src/trusted_roots.pem", "src/letsencrypt.pem");

get_file("https://www.nifty-networks.net/swirc/curl-8.23.0.cab", "curl-8.23.0.cab");
get_file("https://www.nifty-networks.net/swirc/gnu-bundle-202205.cab", "gnu-bundle-202205.cab");
get_file("https://www.nifty-networks.net/swirc/hunspell-1.7.6.cab", "hunspell-1.7.6.cab");
get_file("https://www.nifty-networks.net/swirc/hunspell-en-us.cab", "hunspell-en-us.cab");
get_file("https://www.nifty-networks.net/swirc/libressl-4.4.0.cab", "libressl-4.4.0.cab");
get_file("https://www.nifty-networks.net/swirc/pdcurses-3.9-utf8-colors.cab", "pdcurses-3.9.cab");
get_file("https://www.nifty-networks.net/swirc/swirc-locales-20261107.cab", "swirc-locales-20261107.cab");
