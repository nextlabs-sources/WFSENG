using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Xml;

namespace AppRegisterInfo
{
    class RegexHelper
    {
        private const string span                   = "</span>";
        private const string LabelAppId             = "<span id=\"ctl00_PlaceHolderMain_LabelAppId\">";
        private const string LabelAppSecret         = "<span id=\"ctl00_PlaceHolderMain_LabelAppSecret\">";
        private const string LabelAppTitle          = "<span id=\"ctl00_PlaceHolderMain_LabelAppTitle\">";
        private const string LabelAppHostUri        = "<span id=\"ctl00_PlaceHolderMain_LabelAppHostUri\">";
        private const string LabelAppRedirectUri    = "<span id=\"ctl00_PlaceHolderMain_LabelAppRedirectUri\">";
        private Regex ClientIdRegex;
        private Match ClientIdMatch;
        private Regex ClientSecretRegex;
        private Match ClientSecretMatch;
        private Regex TitleRegex;
        private Match TitleMatch;
        private Regex AppDomainRegex;
        private Match AppDomainMatch;
        private Regex RedirectURIRegex;
        private Match RedirectURIMatch;

        public bool getAllInfoSuccess
        {
            get
            {
                return !(string.IsNullOrEmpty(this.ClientId)
                        || string.IsNullOrEmpty(this.ClientSecret)
                        || string.IsNullOrEmpty(this.Title)
                        || string.IsNullOrEmpty(this.AppDomain)
                        || string.IsNullOrEmpty(this.RedirectURI));
            }
        }

        public string ClientId 
        { 
            get
            {
                if (ClientIdMatch.Success)
                {
                    string tmp = ClientIdMatch.Value.Remove(ClientIdMatch.Value.IndexOf(LabelAppId), LabelAppId.Length);
                    // Here remove the "</span>"
                    return tmp.Remove(tmp.IndexOf(span));
                }
                return ""; 
            }
            private set { }
        }

        public string ClientSecret 
        {
            get 
            {
                if (ClientSecretMatch.Success)
                {
                    string tmp = ClientSecretMatch.Value.Remove(ClientSecretMatch.Value.IndexOf(LabelAppSecret), LabelAppSecret.Length);
                    // Here remove the "</span>"
                    return tmp.Remove(tmp.IndexOf(span));
                }
                return ""; 
            }
            private set { }
        }

        public string Title 
        {
            get
            {
                if (TitleMatch.Success) 
                {
                    string tmp = TitleMatch.Value.Remove(TitleMatch.Value.IndexOf(LabelAppTitle), LabelAppTitle.Length);
                    // Here remove the "</span>"
                    return tmp.Remove(tmp.IndexOf(span));
                }
                return ""; 
            }
            private set { }
        }

        public string AppDomain 
        {
            get
            {
                if (AppDomainMatch.Success) 
                {
                    string tmp = AppDomainMatch.Value.Remove(AppDomainMatch.Value.IndexOf(LabelAppHostUri), LabelAppHostUri.Length);
                    // Here remove the "</span>"
                    return tmp.Remove(tmp.IndexOf(span));
                }
                return ""; 
            }
            private set { }
        }

        public string RedirectURI 
        {
            get
            {
                if (RedirectURIMatch.Success) 
                {
                    string tmp = RedirectURIMatch.Value.Remove(RedirectURIMatch.Value.IndexOf(LabelAppRedirectUri), LabelAppRedirectUri.Length);
                    // Here remove the "</span>"
                    return tmp.Remove(tmp.IndexOf(span));
                }
                return ""; 
            }
            private set { } 
        }

        public RegexHelper(string innerHTML)
        {
            this.ClientIdRegex      = new Regex(LabelAppId + @"(\d|[a-z]|-)*" + span);
            this.ClientIdMatch      = string.IsNullOrEmpty(innerHTML) ? this.ClientIdRegex.Match(string.Empty)     : this.ClientIdRegex.Match(innerHTML);
            this.ClientSecretRegex  = new Regex(LabelAppSecret + @"(\d|\D){44}" + span);
            this.ClientSecretMatch  = string.IsNullOrEmpty(innerHTML) ? this.ClientSecretRegex.Match(string.Empty) : this.ClientSecretRegex.Match(innerHTML);
            this.TitleRegex         = new Regex(LabelAppTitle + @"\w+" + span);
            this.TitleMatch         = string.IsNullOrEmpty(innerHTML) ? this.TitleRegex.Match(string.Empty)        : this.TitleRegex.Match(innerHTML);
            this.AppDomainRegex     = new Regex(LabelAppHostUri + @"([\w-]+\.)+[\w-]+(/[\w- ./?%&=]*)?" + span);
            this.AppDomainMatch     = string.IsNullOrEmpty(innerHTML) ? this.AppDomainRegex.Match(string.Empty)    : this.AppDomainRegex.Match(innerHTML);
            this.RedirectURIRegex   = new Regex(LabelAppRedirectUri + @"https://((\w+|\d|:)*|([\w-]+\.)+[\w-]+(/[\w- ./?%&=]*)?)" + span);
            this.RedirectURIMatch   = string.IsNullOrEmpty(innerHTML) ? this.RedirectURIRegex.Match(string.Empty)  : this.RedirectURIRegex.Match(innerHTML);
        }
    }

    class XmlHelper
    {
        public static string ClientIdNodeName
        {
            get
            {
                return "<ClientId>";
            }
        }

        public static string ClientSecretNodeName
        {
            get
            {
                return "<ClientSecret>";
            }
        }

        public static string TitleNodeName
        {
            get
            {
                return "<Title>";
            }
        }

        public static string AppDomainNodeName
        {
            get
            {
                return "<AppDomain>";
            }
        }

        public static string RedirectURI
        {
            get
            {
                return "<RedirectURI>";
            }
        }

        public static void writeToAnXml(string xmlPath, RegexHelper info)
        {
            string clientIdLine     = ClientIdNodeName      + info.ClientId     + ClientIdNodeName.Insert(1, "/");
            string clientSecretLine = ClientSecretNodeName  + info.ClientSecret + ClientSecretNodeName.Insert(1, "/");
            string titleLine        = TitleNodeName         + info.Title        + TitleNodeName.Insert(1, "/");
            string appDomainLine    = AppDomainNodeName     + info.AppDomain    + AppDomainNodeName.Insert(1, "/");
            string redirectUri      = RedirectURI           + info.RedirectURI  + RedirectURI.Insert(1, "/");
            string directory        = Path.GetDirectoryName(xmlPath);

            if (!Directory.Exists(directory))
                Directory.CreateDirectory(directory);

            string finalXml = "<root>"                  + Environment.NewLine
                            + "\t" + clientIdLine       + Environment.NewLine
                            + "\t" + clientSecretLine   + Environment.NewLine
                            + "\t" + titleLine          + Environment.NewLine
                            + "\t" + appDomainLine      + Environment.NewLine
                            + "\t" + redirectUri        + Environment.NewLine
                            + "</root>";
            File.WriteAllText(xmlPath, finalXml);
            File.SetAttributes(xmlPath, FileAttributes.Hidden);
        }

        public static void writeToATxt(string txtPath, RegexHelper info)
        {
            string directory = Path.GetDirectoryName(txtPath);

            if (!Directory.Exists(directory))
                Directory.CreateDirectory(directory);

            string finalTxt = "Client Id:\t"        + info.ClientId     + Environment.NewLine
                            + "Client Secret:\t"    + info.ClientSecret + Environment.NewLine
                            + "Title:\t\t"          + info.Title        + Environment.NewLine
                            + "App Domain:\t"       + info.AppDomain    + Environment.NewLine
                            + "Redirect URI:\t"     + info.RedirectURI;
            File.WriteAllText(txtPath, finalTxt);
        }

        public static void writeToATxt_RMS(string txtPath, RegexHelper info, string fullUrl)
        {
            string directory = Path.GetDirectoryName(txtPath);

            if (!Directory.Exists(directory))
                Directory.CreateDirectory(directory);

            string finalTxt = "client_id:\t" + info.ClientId + Environment.NewLine
                            + "client_secret:\t" + info.ClientSecret + Environment.NewLine
                            + "display_name:\t" + fullUrl;
            File.WriteAllText(txtPath, finalTxt);
        }

        public static void setTmp(RegexHelper info)
        {
            // Get the current working directory of this application
            string directory = Environment.CurrentDirectory;
            File.WriteAllText(directory + @"\clientid.config",      info.ClientId);
            File.WriteAllText(directory + @"\clientsecret.config",  info.ClientSecret);
            File.WriteAllText(directory + @"\title.config",         info.Title);
            File.WriteAllText(directory + @"\appdomain.config",     info.AppDomain);
            File.WriteAllText(directory + @"\redirecturi.config",   info.RedirectURI);
        }

        public static void setTmp(string clientId, string clientSecret, string title, string appDomain, string redirectUri)
        {
            // Get the current working directory of this application
            string directory = Environment.CurrentDirectory;
            File.WriteAllText(directory + @"\clientid.config",      clientId);
            File.WriteAllText(directory + @"\clientsecret.config",  clientSecret);
            File.WriteAllText(directory + @"\title.config",         title);
            File.WriteAllText(directory + @"\appdomain.config",     appDomain);
            File.WriteAllText(directory + @"\redirecturi.config",   redirectUri);
        }

        public static void readAppInfoFromXml(string xmlPath, out string clientId, out string clientSecret, out string title, out string appDomain, out string redirectUri)
        {
            clientId = clientSecret = title = appDomain = redirectUri = null;
            XmlDocument doc = new XmlDocument();
            doc.Load(@"c:\automation\xmlTest.xml");
            XmlElement root = doc.DocumentElement;
            XmlNodeList nodes = root.ChildNodes;

            foreach (XmlNode node in nodes)
            {
                switch (node.Name.ToLower())
                {
                    case "clientid":
                        clientId = node.InnerText;
                        break;
                    case "clientsecret":
                        clientSecret = node.InnerText;
                        break;
                    case "title":
                        title = node.InnerText;
                        break;
                    case"appdomain":
                        appDomain = node.InnerText;
                        break;
                    case"redirecturi":
                        redirectUri = node.InnerText;
                        break;
                }
            }
        }
    }
}
