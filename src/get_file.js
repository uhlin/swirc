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

get_file("http://nifty-networks.net/swirc/curl-7.62.0.cab", "curl-7.62.0.cab");
get_file("http://nifty-networks.net/swirc/libressl-2.8.2-windows.cab", "libressl-2.8.2-windows.cab");
get_file("http://nifty-networks.net/swirc/pdcurses-3.8.cab", "pdcurses-3.8.cab");
get_file("http://nifty-networks.net/swirc/swirc-royal.ico", "swirc-royal.ico");
