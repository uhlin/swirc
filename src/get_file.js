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

// get_file("http://nifty-networks.net/swirc/blue-globe.ico", "blue-globe.ico");
get_file("http://nifty-networks.net/swirc/libressl-2.5.4-windows.cab", "libressl-2.5.4-windows.cab");
get_file("http://nifty-networks.net/swirc/pdcurses-3.4.cab", "pdcurses-3.4.cab");
get_file("http://nifty-networks.net/swirc/swirc-royal.ico", "swirc-royal.ico");
