/*
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Dan Smith <danms@us.ibm.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>

static int tcp_incoming(int port)
{
        struct sockaddr_in srvaddr;
        int s = -1;
        int ret;

        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) {
                perror("socket");
                return -1;
        }

        memset(&srvaddr, 0, sizeof(srvaddr));

        srvaddr.sin_family = AF_INET;
        srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        srvaddr.sin_port = htons(port);

        ret = bind(s, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
        if (ret) {
                perror("bind");
                goto err;
        }

        ret = listen(s, 1);
        if (ret) {
                perror("listen");
                goto err;
        }

        return s;

 err:
        close(s);
        return -1;
}

static int create_filter(FILE *pipe,
                         const char *name,
                         const char *type,
                         const char *ns)
{
        int ret = 0;
        char *xml = NULL;
        const char *filter_fmt = " \
<?xml version=\"1.0\" encoding=\"utf-8\"?> \
<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\"> \
  <MESSAGE ID=\"4711\" PROTOCOLVERSION=\"1.0\"> \
    <SIMPLEREQ> \
      <IMETHODCALL NAME=\"CreateInstance\"> \
        <LOCALNAMESPACEPATH> \
          <NAMESPACE NAME=\"root\"/> \
          <NAMESPACE NAME=\"PG_InterOp\"/> \
        </LOCALNAMESPACEPATH> \
        <IPARAMVALUE NAME=\"NewInstance\"> \
          <INSTANCE CLASSNAME=\"CIM_IndicationFilter\">\
            <PROPERTY NAME=\"SystemCreationClassName\" TYPE=\"string\"> \
              <VALUE>CIM_ComputerSystem</VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"SystemName\" TYPE=\"string\"> \
              <VALUE>localhost.localdomain</VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"CreationClassName\" TYPE=\"string\"> \
              <VALUE>CIM_IndicationFilter</VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"Name\" TYPE=\"string\"> \
              <VALUE>%sFilter</VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"Query\" TYPE=\"string\"> \
              <VALUE> SELECT * FROM %s \
              </VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"QueryLanguage\" TYPE=\"string\"> \
              <VALUE>WQL</VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"SourceNamespace\" TYPE=\"string\"> \
              <VALUE>%s</VALUE> \
            </PROPERTY> \
          </INSTANCE> \
        </IPARAMVALUE> \
      </IMETHODCALL> \
    </SIMPLEREQ> \
  </MESSAGE> \
</CIM>";

        ret = asprintf(&xml, filter_fmt, name, type, ns);
        if (ret == -1)
                goto out;

        ret = fwrite(xml, strlen(xml), 1, pipe);
        ret = (ret == strlen(xml));

 out:
        free(xml);

        return ret;
}

static int create_handler(FILE *pipe, int port, const char *name)
{
        int ret = 0;
        char *xml = NULL;
        const char * handler_fmt = " \
<?xml version=\"1.0\" encoding=\"utf-8\"?> \
<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\"> \
  <MESSAGE ID=\"4711\" PROTOCOLVERSION=\"1.0\"> \
    <SIMPLEREQ> \
      <IMETHODCALL NAME=\"CreateInstance\"> \
        <LOCALNAMESPACEPATH> \
          <NAMESPACE NAME=\"root\"/> \
          <NAMESPACE NAME=\"PG_InterOp\"/> \
        </LOCALNAMESPACEPATH> \
        <IPARAMVALUE NAME=\"NewInstance\"> \
          <INSTANCE CLASSNAME=\"CIM_IndicationHandlerCIMXML\"> \
            <PROPERTY NAME=\"SystemCreationClassName\" TYPE=\"string\"> \
              <VALUE>CIM_ComputerSystem</VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"SystemName\" TYPE=\"string\"> \
              <VALUE>localhost.localdomain</VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"CreationClassName\" TYPE=\"string\"> \
              <VALUE>CIM_IndicationHandlerCIMXML</VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"Name\" TYPE=\"string\"> \
              <VALUE>%sHandler</VALUE> \
            </PROPERTY> \
            <PROPERTY NAME=\"Destination\" TYPE=\"string\"> \
              <VALUE>localhost:%i</VALUE> \
            </PROPERTY> \
          </INSTANCE> \
        </IPARAMVALUE> \
      </IMETHODCALL> \
    </SIMPLEREQ> \
  </MESSAGE> \
</CIM>";

        ret = asprintf(&xml, handler_fmt, name, port);
        if (ret == -1)
                return 0;

        ret = fwrite(xml, strlen(xml), 1, pipe);
        ret = (ret == strlen(xml));

        free(xml);

        return ret;
}

static int create_subscription(FILE *pipe, const char *name)
{
        int ret = 0;
        char *xml = NULL;
        const char *sub_fmt = " \
<?xml version=\"1.0\" encoding=\"utf-8\"?> \
<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\"> \
  <MESSAGE ID=\"4711\" PROTOCOLVERSION=\"1.0\"> \
    <SIMPLEREQ> \
      <IMETHODCALL NAME=\"CreateInstance\"> \
        <LOCALNAMESPACEPATH> \
          <NAMESPACE NAME=\"root\"/> \
          <NAMESPACE NAME=\"PG_InterOp\"/> \
        </LOCALNAMESPACEPATH> \
        <IPARAMVALUE NAME=\"NewInstance\"> \
          <INSTANCE CLASSNAME=\"CIM_IndicationSubscription\"> \
            <PROPERTY.REFERENCE NAME=\"Filter\" \
                                REFERENCECLASS=\"CIM_IndicationFilter\"> \
              <VALUE.REFERENCE> \
                <INSTANCENAME CLASSNAME=\"CIM_IndicationFilter\"> \
                  <KEYBINDING NAME=\"SystemCreationClassName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    CIM_ComputerSystem \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"SystemName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    localhost.localdomain \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"CreationClassName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    CIM_IndicationFilter \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"Name\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    %sFilter \
                    </KEYVALUE> \
                  </KEYBINDING> \
                </INSTANCENAME> \
              </VALUE.REFERENCE> \
            </PROPERTY.REFERENCE> \
            <PROPERTY.REFERENCE NAME=\"Handler\" \
                                REFERENCECLASS=\"CIM_IndicationHandler\"> \
              <VALUE.REFERENCE> \
                <INSTANCENAME CLASSNAME=\"CIM_IndicationHandlerCIMXML\"> \
                  <KEYBINDING NAME=\"SystemCreationClassName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    CIM_ComputerSystem \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"SystemName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    localhost.localdomain \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"CreationClassName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    CIM_IndicationHandlerCIMXML \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"Name\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    %sHandler \
                    </KEYVALUE> \
                  </KEYBINDING> \
                </INSTANCENAME> \
              </VALUE.REFERENCE> \
            </PROPERTY.REFERENCE> \
            <PROPERTY NAME=\"SubscriptionState\" TYPE=\"uint16\"> \
              <VALUE> 2 </VALUE> \
            </PROPERTY> \
          </INSTANCE> \
        </IPARAMVALUE> \
      </IMETHODCALL> \
    </SIMPLEREQ> \
  </MESSAGE> \
</CIM>";

        ret = asprintf(&xml, sub_fmt, name, name);
        if (ret == -1)
                goto out;

        ret = fwrite(xml, strlen(xml), 1, pipe);
        ret = (ret == strlen(xml));

 out:
        free(xml);

        return ret;
}

static int delete_inst(FILE *pipe, const char *name, const char *type)
{
        int ret;
        char *xml = NULL;
        const char *fmt = "\
<?xml version=\"1.0\" encoding=\"utf-8\"?> \
<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\"> \
  <MESSAGE ID=\"4711\" PROTOCOLVERSION=\"1.0\"> \
    <SIMPLEREQ> \
      <IMETHODCALL NAME=\"DeleteInstance\"> \
        <LOCALNAMESPACEPATH> \
          <NAMESPACE NAME=\"root\"/> \
          <NAMESPACE NAME=\"PG_InterOp\"/> \
        </LOCALNAMESPACEPATH> \
        <IPARAMVALUE NAME=\"InstanceName\"> \
          <INSTANCENAME CLASSNAME=\"CIM_Indication%sCIMXML\"> \
            <KEYBINDING NAME=\"SystemCreationClassName\"> \
              <KEYVALUE>CIM_ComputerSystem</KEYVALUE> \
            </KEYBINDING> \
            <KEYBINDING NAME=\"SystemName\"> \
              <KEYVALUE>localhost.localdomain</KEYVALUE> \
            </KEYBINDING> \
            <KEYBINDING NAME=\"CreationClassName\"> \
              <KEYVALUE>CIM_Indication%sCIMXML</KEYVALUE> \
            </KEYBINDING> \
            <KEYBINDING NAME=\"Name\"> \
              <KEYVALUE>%s%s</KEYVALUE> \
            </KEYBINDING> \
          </INSTANCENAME> \
        </IPARAMVALUE> \
      </IMETHODCALL> \
    </SIMPLEREQ> \
  </MESSAGE> \
</CIM>";

        ret = asprintf(&xml, fmt, type, type, name, type);
        if (ret == -1)
                return 0;

        ret = fwrite(xml, strlen(xml), 1, pipe);
        ret = (ret == strlen(xml));

        free(xml);

        return ret;
}

static int delete_subscription(FILE *pipe, char *name)
{
        int ret;
        char *xml = NULL;
        const char *fmt = "\
<?xml version=\"1.0\" encoding=\"utf-8\"?> \
<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\"> \
  <MESSAGE ID=\"4711\" PROTOCOLVERSION=\"1.0\"> \
    <SIMPLEREQ> \
      <IMETHODCALL NAME=\"DeleteInstance\"> \
        <LOCALNAMESPACEPATH> \
          <NAMESPACE NAME=\"root\"/> \
          <NAMESPACE NAME=\"PG_InterOp\"/> \
        </LOCALNAMESPACEPATH> \
        <IPARAMVALUE NAME=\"InstanceName\"> \
          <INSTANCENAME CLASSNAME=\"CIM_IndicationSubscription\"> \
            <KEYBINDING NAME=\"Filter\"> \
              <VALUE.REFERENCE> \
                <INSTANCENAME CLASSNAME=\"CIM_IndicationFilter\"> \
                  <KEYBINDING NAME=\"SystemCreationClassName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    CIM_ComputerSystem \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"SystemName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    localhost.localdomain \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"CreationClassName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    CIM_IndicationFilter \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"Name\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    %sFilter \
                    </KEYVALUE> \
                  </KEYBINDING> \
                </INSTANCENAME> \
              </VALUE.REFERENCE> \
            </KEYBINDING> \
            <KEYBINDING NAME=\"Handler\"> \
              <VALUE.REFERENCE> \
                <INSTANCENAME CLASSNAME=\"CIM_IndicationHandlerCIMXML\"> \
                  <KEYBINDING NAME=\"SystemCreationClassName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    CIM_ComputerSystem \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"SystemName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    localhost.localdomain \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"CreationClassName\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    CIM_IndicationHandlerCIMXML \
                    </KEYVALUE> \
                  </KEYBINDING> \
                  <KEYBINDING NAME=\"Name\"> \
                    <KEYVALUE VALUETYPE=\"string\"> \
                    %sHandler \
                    </KEYVALUE> \
                  </KEYBINDING> \
                </INSTANCENAME> \
              </VALUE.REFERENCE> \
            </KEYBINDING> \
          </INSTANCENAME> \
        </IPARAMVALUE> \
      </IMETHODCALL> \
    </SIMPLEREQ> \
  </MESSAGE> \
</CIM>";

        ret = asprintf(&xml, fmt, name, name);
        if (ret == -1)
                return 0;

        ret = fwrite(xml, strlen(xml), 1, pipe);
        ret = (ret == strlen(xml));

        free(xml);

        return 1;
}

int interrupted;

static int monitor_indications(int fd, int howmany)
{
        int i;
        char buf[4096];
        struct sockaddr saddr;
        unsigned int len;

        int flags;

        flags = fcntl(fd, F_GETFD);
        fcntl(fd, F_SETFD, flags | O_NONBLOCK);

        interrupted = 0;

        for (i = 0; i < howmany; i++) {
                int ret;
                int s;
                fd_set fds;

                if (interrupted)
                        break;

                FD_ZERO(&fds);
                FD_SET(fd, &fds);

                ret = select(fd+1, &fds, NULL, NULL, NULL);
                if (ret <= 0)
                        continue;

                s = accept(fd, &saddr, &len);
                if (s < 0)
                        continue;

                ret = read(s, buf, sizeof(buf));
                if (ret > 0)
                        printf("Got indication\n");

                close(s);
        }

        return i;
}

static void interrupt(int sig)
{
        interrupted = 1;
}

struct config {
        char *type;
        int port;
        char *name;
        char *ns;
        int iter;
        int debug;
};

static void usage(char *name)
{
        printf("Usage:\n"
               "%s [OPTIONS]\n"
               "\n"
               "  -t,--type=class     Indication type to watch for\n"
               "  -p,--port=1234      TCP port to listen to\n"
               "  -n,--name=foo       Indication instance name base\n"
               "  -N,--ns=foo         Namespace\n"
               "  -i,--iter=1         Number of instances to receive before exit\n"
               "  -d,--debug          Enable debugging\n"
               "  -h,--help           This help text\n",
               name);
}

static int parse_args(int argc, char **argv, struct config *config)
{
        int idx;
        static struct option opts[] = {
                {"type",   1, 0, 't'},
                {"port",   1, 0, 'p'},
                {"name",   1, 0, 'n'},
                {"ns",     1, 0, 'N'},
                {"iter",   1, 0, 'i'},
                {"help",   0, 0, 'h'},
                {"debug",  0, 0, 'd'},
                {0,        0, 0, 0}};

        config->type = "CIM_InstCreation";
        config->port = 1234;
        config->name = "TestIndication";
        config->ns = "root/ibmsd";
        config->iter = 1;
        config->debug = 0;

        while (1) {
                int c;

                c = getopt_long(argc, argv, "t:p:n:N:i:hd", opts, &idx);
                if (c == -1)
                        break;

                switch (c) {
                case 't':
                        config->type = optarg;
                        break;

                case'n':
                        config->name = optarg;
                        break;

                case 'N':
                        config->ns = optarg;
                        break;

                case 'p':
                        config->port = atoi(optarg);
                        break;

                case 'i':
                        config->iter = atoi(optarg);
                        break;

                case 'd':
                        config->debug = 1;
                        break;

                case 'h':
                case '?':
                        usage(argv[0]);
                        return 0;
                        break;
                };
        }

        return 1;
}

#define WBEMCAT(f)                                              \
        if (config.debug)                                       \
                pipe = popen("wbemcat", "w");                   \
        else                                                    \
                pipe = popen("wbemcat >/dev/null 2>&1", "w");   \
        f;                                                      \
        ret = fclose(pipe);

int main(int argc, char **argv)
{
        int s;
        FILE *pipe;
        int ret;
        struct config config;

        if (!parse_args(argc, argv, &config))
                return 1;

        signal(SIGINT, interrupt);
        signal(SIGTERM, interrupt);

        s = tcp_incoming(config.port);
        if (s < 0) {
                printf("Unable to listen on port %i\n", config.port);
                return 1;
        }

        WBEMCAT(create_filter(pipe, config.name, config.type, config.ns));
        if (ret) {
                printf("Failed to create filter\n");
                return 1;
        }

        WBEMCAT(create_handler(pipe, config.port, config.name));
        if (ret) {
                printf("Failed to create handler\n");
                goto out1;
        }

        WBEMCAT(create_subscription(pipe, config.name));
        if (ret) {
                printf("Failed to create subscription\n");
                goto out2;
        }

        monitor_indications(s, config.iter);

        WBEMCAT(delete_subscription(pipe, config.name));
 out2:
        WBEMCAT(delete_inst(pipe, config.name, "Handler"));
 out1:
        WBEMCAT(delete_inst(pipe, config.name, "Filter"));

        close(s);

        return 0;
}

/*
 * Local Variables:
 * mode: C
 * c-set-style: "K&R"
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * End:
 */
