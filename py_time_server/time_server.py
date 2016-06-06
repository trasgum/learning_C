from time import gmtime, strftime
import http.server
import socketserver

class myHandler(http.server.BaseHTTPRequestHandler):

    # Handler for the GET requests
    def do_GET(self):
        # print   ('Get request received')
        self.send_response(200)
        self.send_header('Content-type','text/html')
        self.send_header('Posix-time', strftime("%s", gmtime()))
        self.end_headers()
        # Send the html message
        self.wfile.write("Time from truco server!!".encode())
        #self.wfile.write(strftime("%s", gmtime()).encode())
        return

PORT = 8000

Handler = myHandler

httpd = socketserver.TCPServer(("", PORT), Handler)

print("serving at port", PORT)
print(strftime("%s", gmtime()))
httpd.serve_forever()

