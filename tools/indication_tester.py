# Copyright IBM Corp. 2007
#!/usr/bin/python
#
# indication_tester.py - Tool for testing indication subscription and
#                        delivery against a CIMOM
# Author: Dan Smith <danms@us.ibm.com>

import sys
from optparse import OptionParser
import BaseHTTPServer
import httplib
from xml.dom.minidom import parse, parseString

def filter_xml(name, type, ns):
    return """
    <?xml version="1.0" encoding="utf-8"?> 
    <CIM CIMVERSION="2.0" DTDVERSION="2.0"> 
    <MESSAGE ID="4711" PROTOCOLVERSION="1.0"> 
      <SIMPLEREQ> 
        <IMETHODCALL NAME="CreateInstance"> 
          <LOCALNAMESPACEPATH> 
            <NAMESPACE NAME="root"/> 
            <NAMESPACE NAME="PG_InterOp"/> 
          </LOCALNAMESPACEPATH> 
          <IPARAMVALUE NAME="NewInstance"> 
              <INSTANCE CLASSNAME="CIM_IndicationFilter">
                <PROPERTY NAME="SystemCreationClassName" TYPE="string"> 
                  <VALUE>CIM_ComputerSystem</VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="SystemName" TYPE="string"> 
                  <VALUE>localhost.localdomain</VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="CreationClassName" TYPE="string"> 
                  <VALUE>CIM_IndicationFilter</VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="Name" TYPE="string"> 
                  <VALUE>%sFilter</VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="Query" TYPE="string"> 
                  <VALUE> SELECT * FROM %s 
                  </VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="QueryLanguage" TYPE="string"> 
                  <VALUE>WQL</VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="SourceNamespace" TYPE="string"> 
                  <VALUE>%s</VALUE> 
                </PROPERTY> 
              </INSTANCE> 
            </IPARAMVALUE> 
          </IMETHODCALL> 
        </SIMPLEREQ> 
      </MESSAGE> 
    </CIM>
    """ % (name, type, ns)

def handler_xml(name, port):
    return """
    <?xml version="1.0" encoding="utf-8"?> 
    <CIM CIMVERSION="2.0" DTDVERSION="2.0"> 
      <MESSAGE ID="4711" PROTOCOLVERSION="1.0"> 
        <SIMPLEREQ> 
        <IMETHODCALL NAME="CreateInstance"> 
            <LOCALNAMESPACEPATH> 
              <NAMESPACE NAME="root"/> 
              <NAMESPACE NAME="PG_InterOp"/> 
            </LOCALNAMESPACEPATH> 
            <IPARAMVALUE NAME="NewInstance"> 
              <INSTANCE CLASSNAME="CIM_IndicationHandlerCIMXML"> 
                <PROPERTY NAME="SystemCreationClassName" TYPE="string"> 
                  <VALUE>CIM_ComputerSystem</VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="SystemName" TYPE="string"> 
                  <VALUE>localhost.localdomain</VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="CreationClassName" TYPE="string"> 
                  <VALUE>CIM_IndicationHandlerCIMXML</VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="Name" TYPE="string"> 
                  <VALUE>%sHandler</VALUE> 
                </PROPERTY> 
                <PROPERTY NAME="Destination" TYPE="string"> 
                  <VALUE>localhost:%i</VALUE> 
                </PROPERTY> 
              </INSTANCE> 
            </IPARAMVALUE> 
          </IMETHODCALL> 
        </SIMPLEREQ> 
      </MESSAGE> 
      </CIM>
      """ % (name, port)

def subscription_xml(name):
    return """
    <?xml version="1.0" encoding="utf-8"?> 
    <CIM CIMVERSION="2.0" DTDVERSION="2.0"> 
      <MESSAGE ID="4711" PROTOCOLVERSION="1.0"> 
        <SIMPLEREQ> 
          <IMETHODCALL NAME="CreateInstance"> 
            <LOCALNAMESPACEPATH> 
              <NAMESPACE NAME="root"/> 
              <NAMESPACE NAME="PG_InterOp"/> 
            </LOCALNAMESPACEPATH> 
            <IPARAMVALUE NAME="NewInstance"> 
              <INSTANCE CLASSNAME="CIM_IndicationSubscription"> 
                <PROPERTY.REFERENCE NAME="Filter" 
                                    REFERENCECLASS="CIM_IndicationFilter"> 
                  <VALUE.REFERENCE> 
                    <INSTANCENAME CLASSNAME="CIM_IndicationFilter"> 
                      <KEYBINDING NAME="SystemCreationClassName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        CIM_ComputerSystem 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="SystemName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        localhost.localdomain 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="CreationClassName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        CIM_IndicationFilter 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="Name"> 
                        <KEYVALUE VALUETYPE="string"> 
                        %sFilter 
                        </KEYVALUE> 
                      </KEYBINDING> 
                    </INSTANCENAME> 
                  </VALUE.REFERENCE> 
                </PROPERTY.REFERENCE> 
                <PROPERTY.REFERENCE NAME="Handler" 
                                    REFERENCECLASS="CIM_IndicationHandler"> 
                  <VALUE.REFERENCE> 
                    <INSTANCENAME CLASSNAME="CIM_IndicationHandlerCIMXML"> 
                      <KEYBINDING NAME="SystemCreationClassName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        CIM_ComputerSystem 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="SystemName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        localhost.localdomain 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="CreationClassName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        CIM_IndicationHandlerCIMXML 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="Name"> 
                        <KEYVALUE VALUETYPE="string"> 
                        %sHandler 
                        </KEYVALUE> 
                      </KEYBINDING> 
                    </INSTANCENAME> 
                  </VALUE.REFERENCE> 
                </PROPERTY.REFERENCE> 
                <PROPERTY NAME="SubscriptionState" TYPE="uint16"> 
                  <VALUE> 2 </VALUE> 
                </PROPERTY> 
              </INSTANCE> 
            </IPARAMVALUE> 
          </IMETHODCALL> 
        </SIMPLEREQ> 
      </MESSAGE> 
      </CIM>
      """ % (name, name)

def delete_inst_xml(name, type):
    return """
    <?xml version="1.0" encoding="utf-8"?> 
    <CIM CIMVERSION="2.0" DTDVERSION="2.0"> 
      <MESSAGE ID="4711" PROTOCOLVERSION="1.0"> 
        <SIMPLEREQ> 
          <IMETHODCALL NAME="DeleteInstance"> 
            <LOCALNAMESPACEPATH> 
              <NAMESPACE NAME="root"/> 
              <NAMESPACE NAME="PG_InterOp"/> 
            </LOCALNAMESPACEPATH> 
            <IPARAMVALUE NAME="InstanceName"> 
              <INSTANCENAME CLASSNAME="CIM_Indication%sCIMXML"> 
                <KEYBINDING NAME="SystemCreationClassName"> 
                  <KEYVALUE>CIM_ComputerSystem</KEYVALUE> 
                </KEYBINDING> 
                <KEYBINDING NAME="SystemName"> 
                  <KEYVALUE>localhost.localdomain</KEYVALUE> 
                </KEYBINDING> 
                <KEYBINDING NAME="CreationClassName"> 
                  <KEYVALUE>CIM_Indication%sCIMXML</KEYVALUE> 
                </KEYBINDING> 
                <KEYBINDING NAME="Name"> 
                  <KEYVALUE>%s%s</KEYVALUE> 
                </KEYBINDING> 
              </INSTANCENAME> 
            </IPARAMVALUE> 
          </IMETHODCALL> 
        </SIMPLEREQ> 
      </MESSAGE> 
    </CIM>;
    """ % (type, type, name, type);

def delete_sub_xml(name):
    return """
    <?xml version="1.0" encoding="utf-8"?> 
    <CIM CIMVERSION="2.0" DTDVERSION="2.0"> 
      <MESSAGE ID="4711" PROTOCOLVERSION="1.0"> 
        <SIMPLEREQ> 
          <IMETHODCALL NAME="DeleteInstance"> 
            <LOCALNAMESPACEPATH> 
              <NAMESPACE NAME="root"/> 
              <NAMESPACE NAME="PG_InterOp"/> 
            </LOCALNAMESPACEPATH> 
            <IPARAMVALUE NAME="InstanceName"> 
              <INSTANCENAME CLASSNAME="CIM_IndicationSubscription"> 
                <KEYBINDING NAME="Filter"> 
                  <VALUE.REFERENCE> 
                    <INSTANCENAME CLASSNAME="CIM_IndicationFilter"> 
                      <KEYBINDING NAME="SystemCreationClassName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        CIM_ComputerSystem 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="SystemName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        localhost.localdomain 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="CreationClassName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        CIM_IndicationFilter 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="Name"> 
                        <KEYVALUE VALUETYPE="string"> 
                        %sFilter 
                        </KEYVALUE> 
                      </KEYBINDING> 
                    </INSTANCENAME> 
                  </VALUE.REFERENCE> 
                </KEYBINDING> 
                <KEYBINDING NAME="Handler"> 
                  <VALUE.REFERENCE> 
                    <INSTANCENAME CLASSNAME="CIM_IndicationHandlerCIMXML"> 
                      <KEYBINDING NAME="SystemCreationClassName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        CIM_ComputerSystem 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="SystemName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        localhost.localdomain 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="CreationClassName"> 
                        <KEYVALUE VALUETYPE="string"> 
                        CIM_IndicationHandlerCIMXML 
                        </KEYVALUE> 
                      </KEYBINDING> 
                      <KEYBINDING NAME="Name"> 
                        <KEYVALUE VALUETYPE="string"> 
                        %sHandler 
                        </KEYVALUE> 
                      </KEYBINDING> 
                    </INSTANCENAME> 
                  </VALUE.REFERENCE> 
                </KEYBINDING> 
              </INSTANCENAME> 
            </IPARAMVALUE> 
          </IMETHODCALL> 
        </SIMPLEREQ> 
      </MESSAGE> 
    </CIM>;
    """ % (name, name)

class CIMIndication:
    def __init__(self, xmldata):
        dom = parseString(xmldata)

        instances = dom.getElementsByTagName("INSTANCE")
        attrs = instances[0].attributes.items()
        self.name = attrs[0][1]

    def __str__(self):
        return self.name

class CIMSocketHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_POST(self):
        length = self.headers.getheader('content-length')
        data = self.rfile.read(int(length))

        indication = CIMIndication(data)
        print "Got indication: %s" % indication

class CIMIndicationSubscription:
    def __init__(self, name, typ, ns):
        self.name = name
        self.type = typ
        self.ns = ns

        self.server = BaseHTTPServer.HTTPServer(('', 8000), CIMSocketHandler)
        self.port = 8000

        self.filter_xml = filter_xml(name, typ, ns)
        self.handler_xml = handler_xml(name, self.port)
        self.subscription_xml = subscription_xml(name)

    def __do_cimpost(self, conn, body):
        conn.request("POST", "/cimom", body, {"CIMOperation" : "MethodCall"})
        resp = conn.getresponse()

        resp.read()

    def subscribe(self, url):
        self.conn = httplib.HTTPConnection(url)

        self.__do_cimpost(self.conn, self.filter_xml)
        self.__do_cimpost(self.conn, self.handler_xml)
        self.__do_cimpost(self.conn, self.subscription_xml)

    def unsubscribe(self):
        self.__do_cimpost(self.conn, delete_sub_xml(self.name))
        self.__do_cimpost(self.conn, delete_inst_xml(self.name, "Handler"))
        self.__do_cimpost(self.conn, delete_inst_xml(self.name, "Filter"))

def main():
    parser = OptionParser()
    parser.add_option("-u", "--url", dest="url",
                      help="URL of CIMOM to connect to (host:port)")
    parser.add_option("-N", "--ns", dest="ns",
                      help="Namespace (default is root/ibmsd)")
    parser.add_option("-n", "--name", dest="name",
                      help="Base name for filter, handler, subscription")

    (options, args) = parser.parse_args()
    if not options.url:
        options.url = "localhost:5988"
    if not options.ns:
        options.ns = "root/ibmsd"
    if not options.name:
        options.name = "Test"

    sub = CIMIndicationSubscription(options.name, args[0], options.ns)
    sub.subscribe(options.url)
    print "Watching for %s" % args[0]

    try:
        sub.server.serve_forever()
    except KeyboardInterrupt,e:
        sub.unsubscribe()
        print "Cancelling subscription for %s" % args[0]

if __name__=="__main__":
    sys.exit(main())
