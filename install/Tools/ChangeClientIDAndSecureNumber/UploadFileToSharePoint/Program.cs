/*
* 测试：
*  能够上传 非空文件 到share point list： document， custom 中
*  命令行工具， 功能正确， 错误不检查（return）
*/

using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Security;
using Microsoft.SharePoint.Client;
using Newtonsoft.Json;
using System.Text.RegularExpressions;
using Nextlabs.Solution.Helpers;
using SPOEBatchWorker;
using Nextlabs.Tools;



namespace UploadFileToSharePoint
{
    class Program
    {
        private static AccessTokenResponse accessTokenResponse;
        private static AccessTokenResponse RFaccessTokenResponse;
        private const string strAppDescription = "© 2015 NextLabs Inc.";

        static void Main(string[] args)
        {
            try
            {
                int count = args.Length;
                string strWebFullURL = args[0].ToString(); //"https://nextlabssolutions.sharepoint.com/sites/KimTest";

                string strListName = args[1].ToString(); //"CavalryTest";

                List<string> strFilePaths = new List<string>();
                for (int i = 2; i < count; i++)
                {
                    strFilePaths.Add(args[i].ToString());
                }

                //string strFilePath      = args[2].ToString(); //"C:\\Users\\cpeng\\Desktop\\test1.docx";

                string strClientId = null; //"ba35c280-ef96-4170-9dde-b688f7f54fe0";

                string strClientSecret = null; //"Baz5kkZEqroNcgIteV%2bRtRvGrPG9JewozsEJVjb1Vko%3d";

                string strRedirectUri = null; //"http://localhost:8989/";

                //string strLogName = args[3].ToString();//"KimYang@nextlabs.onmicrosoft.com";

                //string strSecureNumber = args[4].ToString();//"123blue!";

                Dictionary<string, string> dic = RegisterHelper.ReadFromRegister(RegistryKeyHelper.rootKey_CurrentUser, RegistryKeyHelper.subKey_SPOE);
                string value;
                foreach (string name in dic.Keys)
                {
                    switch (name.ToLower())
                    {
                        case "clientid":
                            if (dic.TryGetValue(name, out value))
                                strClientId = value;
                            else
                                strClientId = string.Empty;
                            break;
                        case "clientsecret":
                            if (dic.TryGetValue(name, out value))
                                strClientSecret = value;
                            else
                                strClientSecret = string.Empty;
                            break;
                        case "redirecturi":
                            if (dic.TryGetValue(name, out value))
                                strRedirectUri = value;
                            else
                                strRedirectUri = string.Empty;
                            break;

                    }
                }

                //UploadFileToSharePointList(strWebFullURL, strFilePaths, strListName, strClientId, strClientSecret, strRedirectUrl);
                string strSPHost = strWebFullURL;
                if (strWebFullURL.LastIndexOf('/') != 7)
                {
                    Regex regex = new Regex(@"https://([\w-]+\.)+[\w-]+/");
                    Match match = regex.Match(strWebFullURL);
                    if (match.Success)
                        strSPHost = match.Value;
                }
                #region delete
                //RestAPIAuth.SetParameter(strSPHost, strClientId, strClientSecret);
                //RestAPIAuth.Login();
                //ClientContext ctx = RestAPIAuth.GetClientContextWithAccessToken(strWebFullURL);

                //Console.WriteLine("1");
                //Web web = ctx.Web;
                //ctx.Load(web, w => w.Title);
                //Console.WriteLine("2");
                //ctx.ExecuteQuery();
                //Console.WriteLine("web title is :" + web.Title);
                //if (!LibraryExists(ctx, web, strListName))
                //{
                //    throw new Exception("List does not exist!");
                //}
                //foreach (string strFilePath in strFilePaths)
                //{
                //    using (FileStream fs = new FileStream(strFilePath, FileMode.Open))
                //    {
                //        FileCreationInformation fctNewFile = new FileCreationInformation();
                //        fctNewFile.ContentStream = fs;
                //        fctNewFile.Url = System.IO.Path.GetFileName(strFilePath);
                //        fctNewFile.Overwrite = true;

                //        List list = web.Lists.GetByTitle(strListName);
                //        Microsoft.SharePoint.Client.File uploadFile = list.RootFolder.Files.Add(fctNewFile);
                //        Microsoft.SharePoint.Client.ListItem addItem = uploadFile.ListItemAllFields;
                //        addItem.Update();
                //        ctx.ExecuteQuery();
                //        ctx.Load(addItem);
                //        ctx.ExecuteQuery();
                //        Dictionary<string, object> FieldValues = addItem.FieldValues;
                //        foreach (KeyValuePair<string, object> keyValue in FieldValues)
                //        {
                //            if (keyValue.Key.Equals("AppDescription", StringComparison.OrdinalIgnoreCase))
                //            {
                //                System.Diagnostics.Trace.WriteLine("[Upload app]-------Contain field AppDescription true");
                //                addItem["AppDescription"] = strAppDescription;
                //                addItem.Update();
                //                ctx.ExecuteQuery();
                //                break;
                //            }
                //        }

                //        Console.WriteLine("upload success!");
                //    }
                //}
                #endregion

                Console.WriteLine("Please wait for uploading app files to\n{0}", strWebFullURL);
                Uri siteUri = new Uri(strWebFullURL);
                Console.WriteLine("Get realm...");
                string realm = TokenHelper.GetRealmFromTargetUrl(siteUri);
                Console.WriteLine("Get app only access token...");
                string accessToken = TokenHelper.GetAppOnlyAccessToken(
                    TokenHelper.SharePointPrincipal,
                    siteUri.Authority, realm, strClientId, strClientSecret).AccessToken;

                using (var clientContext = TokenHelper.GetClientContextWithAccessToken(
                                                        siteUri.ToString(), 
                                                        accessToken))
                {
                    //CheckListExists(clientContext, strListName);
                    //AddListItem(clientContext, strListName);
                    Web web = clientContext.Web;
                    clientContext.Load(web, w => w.Title);
                    Console.WriteLine("Connect to the site...");
                    clientContext.ExecuteQuery();
                    Console.WriteLine("web title is :" + web.Title);
                    if (!LibraryExists(clientContext, web, strListName))
                    {
                        throw new Exception("List does not exist!");
                    }
                    foreach (string strFilePath in strFilePaths)
                    {
                        using (FileStream fs = new FileStream(strFilePath, FileMode.Open))
                        {
                            FileCreationInformation fctNewFile = new FileCreationInformation();
                            fctNewFile.ContentStream = fs;
                            fctNewFile.Url = System.IO.Path.GetFileName(strFilePath);
                            fctNewFile.Overwrite = true;

                            List list = web.Lists.GetByTitle(strListName);
                            Microsoft.SharePoint.Client.File uploadFile = list.RootFolder.Files.Add(fctNewFile);
                            Microsoft.SharePoint.Client.ListItem addItem = uploadFile.ListItemAllFields;
                            Console.WriteLine("Update the files' info...");
                            addItem.Update();
                            clientContext.ExecuteQuery();
                            Console.WriteLine("Upload the files...");
                            clientContext.Load(addItem);
                            clientContext.ExecuteQuery();
                            Dictionary<string, object> FieldValues = addItem.FieldValues;
                            foreach (KeyValuePair<string, object> keyValue in FieldValues)
                            {
                                if (keyValue.Key.Equals("AppDescription", StringComparison.OrdinalIgnoreCase))
                                {
                                    System.Diagnostics.Trace.WriteLine("[Upload app]-------Contain field AppDescription true");
                                    addItem["AppDescription"] = strAppDescription;
                                    addItem.Update();
                                    clientContext.ExecuteQuery();
                                    break;
                                }
                            }

                            Console.WriteLine("upload success!");
                        }
                    }
                }

                dic.Clear();
                dic.Add("UploadApps", "passed");

                RegistryKeyHelper.WriteToRegister(RegistryKeyHelper.rootKey_CurrentUser, RegistryKeyHelper.subKey_InstallInfo, dic);
            }
            catch (Exception ex)
            {
                Console.WriteLine("error massage is :  " + ex.Message);
                Console.WriteLine("error stack trace : " + ex.StackTrace);
                Console.ReadKey();
            }

        }

        static bool UploadFileToSharePointList(string strWebFullURL, List<string> strFilePaths, string strListName, string strClientId, string strClientSecret, string strRedirectUrl) 
        {
            // Check URL: strListUrl
            try
            {
                Console.WriteLine("test1");
                Uri siteUri = new Uri(strWebFullURL);


                string strID        = strClientId;//"ba35c280-ef96-4170-9dde-b688f7f54fe0";
                string strSecret    = strClientSecret;//"Baz5kkZEqroNcgIteV%2bRtRvGrPG9JewozsEJVjb1Vko%3d";
                string strResource  = "https://api.office.com/discovery/";
                string strReplyUrl  = strRedirectUrl;//"http://localhost:8989/";
                string strCode      = "111";
                Regex reg           = new Regex(@":[\d]+");
                Match match         = reg.Match(strReplyUrl);
                // assign port to the default value: 8989
                int port            = 8989;
                if (!string.IsNullOrEmpty(match.Value))
                {
                    string strPort = match.Value.Remove(0, 1);
                    // get the port from redirect url
                    port = int.Parse(strPort);
                }
                string strRequestforCode = string.Format("https://login.windows.net/common/oauth2/authorize?response_type=code&client_id={0}&redirect_uri={1}&resource={2}&prompt=consent",
                                                         strID, 
                                                         strReplyUrl, 
                                                         strResource);

                Console.WriteLine("strRequest is :" + strRequestforCode);

                System.Diagnostics.Process.Start(strRequestforCode);

                IPAddress localip = IPAddress.Parse("127.0.0.1");
                TcpListener listener = new TcpListener(localip, port);
                listener.Start();
                TcpClient s = listener.AcceptTcpClient();
                NetworkStream stream = s.GetStream();
                byte[] byteRead = new byte[1024];
                stream.Read(byteRead, 0, 1024);
                string strResponseforCode = Encoding.ASCII.GetString(byteRead);

                //response 200 ok
                string strResponseHeader        = "HTTP/1.0 200 OK\r\n";
                strResponseHeader               += "Content-Type:text/html\r\n";
                strResponseHeader               += "Connection:close\r\n";
                string strResponseBody          = "Authentication success and batch worker is running now.";
                strResponseHeader               += "Content-Length:" + strResponseBody.Length.ToString() + "\r\n";
                string strAuthSuccessResponse   = strResponseHeader + "\r\n" + strResponseBody;

                byte[] byteWrite = Encoding.ASCII.GetBytes(strAuthSuccessResponse);
                stream.Write(byteWrite, 0, byteWrite.Length);
                stream.Flush();
                s.Close();
                

                Console.WriteLine("response is : " + strResponseforCode);

                strCode = GetAuthenCodeFromResponse(strResponseforCode);

                Console.WriteLine("response code is :" + strCode);
                //Console.ReadKey();

                string strRequestforToken = string.Format("grant_type=authorization_code&client_id={0}&client_secret={1}&code={2}&resource={3}&redirect_uri={4}",
                    strID, strSecret, strCode, strResource, strReplyUrl);

                HttpWebRequest request = (HttpWebRequest)WebRequest.Create(@"https://login.windows.net/common/oauth2/token");

                request.Accept = @"*/*";
                request.ContentType = @"application/x-www-form-urlencoded";
                request.UserAgent = @"Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)";
                request.Method = "POST";
                request.Referer = string.Empty;
                request.Timeout = 1000 * 60;
                byte[] buffer = new byte[0];
                buffer = Encoding.ASCII.GetBytes(strRequestforToken);

                request.ContentLength = buffer.Length;

                try
                {
                    using (Stream writer = request.GetRequestStream())
                    {
                        writer.Write(buffer, 0, buffer.Length);

                        writer.Flush();
                    }

                    //get response

                    HttpWebResponse response = (HttpWebResponse)request.GetResponse();

                    StreamReader reader = new StreamReader(response.GetResponseStream(), Encoding.ASCII);

                    string strResponse = reader.ReadToEnd();

                    Console.WriteLine("response is : " + strResponse);

                    //get access token from response
                    accessTokenResponse = JsonConvert.DeserializeObject<AccessTokenResponse>(strResponse);

                    //set access token
                    string strAccessToken = accessTokenResponse.access_token;
                    string strRefreshToken = accessTokenResponse.refresh_token;
                    string strTargetUrl = strWebFullURL.Substring(0,strWebFullURL.LastIndexOf(".com/")+5);
                    Console.WriteLine("accessToken is :" + strAccessToken);
                    Console.WriteLine(" ");
                    Console.WriteLine("RefreshToken is :" + strRefreshToken);

                    string strRequestforRefresh = string.Format("grant_type=refresh_token&client_id={0}&client_secret={1}&refresh_token={2}&resource={3}",
                                              strID, strSecret, strRefreshToken, strTargetUrl);


                    HttpWebRequest refreshRequest = (HttpWebRequest)WebRequest.Create(@"https://login.windows.net/common/oauth2/token");

                    refreshRequest.Accept = @"*/*";
                    refreshRequest.ContentType = @"application/x-www-form-urlencoded";
                    refreshRequest.UserAgent = @"Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)";
                    refreshRequest.Method = "POST";
                    refreshRequest.Referer = string.Empty;
                    refreshRequest.Timeout = 1000 * 60;
                    byte[] buffer2 = new byte[0];
                    buffer2 = Encoding.ASCII.GetBytes(strRequestforRefresh);

                    refreshRequest.ContentLength = buffer2.Length;

                    try
                    {
                        using(Stream RFwriter = refreshRequest.GetRequestStream())
                        {
                            RFwriter.Write(buffer2, 0, buffer2.Length);
                            RFwriter.Flush();
                        }

                        HttpWebResponse RFresponse = (HttpWebResponse)refreshRequest.GetResponse();

                        StreamReader RFreader = new StreamReader(RFresponse.GetResponseStream(), Encoding.ASCII);

                        string strRFResponse = RFreader.ReadToEnd();

                        Console.WriteLine("RFResponse is :" + strRFResponse);
                        Console.WriteLine(" ");

                        RFaccessTokenResponse = JsonConvert.DeserializeObject<AccessTokenResponse>(strRFResponse);

                        Console.WriteLine("RFaccToken is :" + RFaccessTokenResponse.access_token);
                        Console.WriteLine(" ");
                        Console.WriteLine("RFaccRefreshToken is :" + RFaccessTokenResponse.refresh_token);

                    }
                    catch(Exception e)
                    {
                        Console.WriteLine("get some error at requestRefreshToken :" + e.Message);
                    }

                    try
                    {

                        ClientContext ctx = GetClientContextWithAccessToken(strWebFullURL, RFaccessTokenResponse.access_token);
                        Console.WriteLine("1");
                        Web web = ctx.Web;
                        ctx.Load(web, w => w.Title);
                        Console.WriteLine("2");
                        ctx.ExecuteQuery();
                        Console.WriteLine("web title is :" + web.Title);
                        if (!LibraryExists(ctx, web, strListName))
                        {
                            throw new Exception("List does not exist!");
                        }
                        foreach (string strFilePath in strFilePaths)
                        {
                            using (FileStream fs = new FileStream(strFilePath, FileMode.Open))
                            {
                                FileCreationInformation fctNewFile = new FileCreationInformation();
                                fctNewFile.ContentStream = fs;
                                fctNewFile.Url = System.IO.Path.GetFileName(strFilePath);
                                fctNewFile.Overwrite = true;

                                List list = web.Lists.GetByTitle(strListName);
                                Microsoft.SharePoint.Client.File uploadFile = list.RootFolder.Files.Add(fctNewFile);
                                Microsoft.SharePoint.Client.ListItem addItem = uploadFile.ListItemAllFields;
                                addItem.Update();
                                ctx.ExecuteQuery();
                                ctx.Load(addItem);
                                ctx.ExecuteQuery();
                                Dictionary<string, object> FieldValues = addItem.FieldValues;
                                foreach (KeyValuePair<string, object> keyValue in FieldValues)
                                {
                                    if (keyValue.Key.Equals("AppDescription", StringComparison.OrdinalIgnoreCase))
                                    {
                                        System.Diagnostics.Trace.WriteLine("[Upload app]-------Contain field AppDescription true");
                                        addItem["AppDescription"] = strAppDescription;
                                        addItem.Update();
                                        ctx.ExecuteQuery();
                                        break;
                                    }
                                }

                                Console.WriteLine("upload success!");
                            }
                        }
                    }
                    catch(Exception e)
                    {
                        Console.WriteLine("get some error is :" + e.Message);
                        Console.ReadKey();

                    }

                        
                    

                }
                catch (Exception except)
                {
                    Console.WriteLine(except.ToString());
                    Console.ReadKey();

                }

            //ClientContext clientcontext = new ClientContext(strWebFullURL);
            //clientcontext.AuthenticationMode = ClientAuthenticationMode.Default;
            //var passWord = new SecureString();
            //foreach (var c in strSecureNumber) passWord.AppendChar(c);
            //var spoc = new SharePointOnlineCredentials(strLogName, passWord);
            //clientcontext.Credentials = spoc;

            //Web web = clientcontext.Site.RootWeb;
            //clientcontext.Load(web, w => w.Title, w => w.Description, w => w.Lists);
            //clientcontext.ExecuteQuery();
            //Console.WriteLine("web.title:{0}", web.Title);
            
            //    if (!LibraryExists(clientcontext, web, strListName))
            //    {
            //        throw new Exception("List does not exist!");
            //    }
            //    using(FileStream fs = new FileStream(strFilePath,FileMode.Open))
            //    {
            //        FileCreationInformation fctNewFile = new FileCreationInformation();
            //        fctNewFile.ContentStream = fs;
            //        fctNewFile.Url = System.IO.Path.GetFileName(strFilePath);
            //        fctNewFile.Overwrite = true;

            //        List list = web.Lists.GetByTitle(strListName);
            //        Microsoft.SharePoint.Client.File uploadFile = list.RootFolder.Files.Add(fctNewFile);

            //        clientcontext.Load(uploadFile);
            //        clientcontext.ExecuteQuery();

            //        Console.WriteLine("upload success!");
            //        Console.Read();
            //    }
            }
            catch(Exception ex)
            {
                Console.WriteLine("error Parameters");
                Console.WriteLine(ex.Message + Environment.NewLine
                                + ex.StackTrace + Environment.NewLine
                                + ex.Source +Environment.NewLine);
                Console.Read();
            }
            
            // trust certification

            // Upload file to list
            return true;
        }

        private static string GetAuthenCodeFromResponse(string strResponse)
        {
            int nBegin = strResponse.IndexOf("code=");
            if (nBegin >= 0)
            {
                int nEnd = strResponse.IndexOf('&', nBegin);
                if (nEnd > nBegin)
                {
                    string strCode = strResponse.Substring(nBegin + 5, nEnd - nBegin - 5);
                    return strCode;
                }

            }
            return "";

        }

        private static bool LibraryExists(ClientContext ctx, Web web, string libraryName)
        {
            ListCollection lists = web.Lists;
            IEnumerable<List> results = ctx.LoadQuery<List>(lists.Where(list => list.Title == libraryName));
            ctx.ExecuteQuery();
            List existingList = results.FirstOrDefault();

            if (existingList != null)
            {
                return true;
            }

            return false;
        }

        public static ClientContext GetClientContextWithAccessToken(string targetUrl, string accessToken)
        {

            ClientContext clientContext = new ClientContext(targetUrl);



            clientContext.AuthenticationMode = ClientAuthenticationMode.Anonymous;

            clientContext.FormDigestHandlingEnabled = false;

            clientContext.ExecutingWebRequest +=

                delegate(object oSender, WebRequestEventArgs webRequestEventArgs)
                {

                    webRequestEventArgs.WebRequestExecutor.RequestHeaders["Authorization"] =

                        "Bearer " + accessToken;

                };



            return clientContext;

        }

        private static void CheckListExists(ClientContext clientContext, string listName)
        {
            ListCollection listCollection = clientContext.Web.Lists;
            clientContext.Load(listCollection, lists => lists.Include(list => list.Title).Where(list => list.Title == listName));
            clientContext.ExecuteQuery();
            if (listCollection.Count <= 0)
            {
                CreateList(clientContext, listName);
            }

        }
        private static void CreateList(ClientContext clientContext, string listName)
        {
            Web currentWeb = clientContext.Web;
            ListCreationInformation creationInfo = new ListCreationInformation();
            creationInfo.Title = listName;
            creationInfo.TemplateType = (int)ListTemplateType.GenericList;
            List list = currentWeb.Lists.Add(creationInfo);
            list.Description = "My custom list";
            list.Update();
            clientContext.ExecuteQuery();
        }
        private static void AddListItem(ClientContext clientContext, string listName)
        {
            Web currentWeb = clientContext.Web;
            var myList = clientContext.Web.Lists.GetByTitle(listName);

            //ListItemCreationInformation listItemCreate = new ListItemCreationInformation();
            //Microsoft.SharePoint.Client.ListItem newItem = myList.AddItem(listItemCreate);

            //Folder folder = currentWeb.GetFolderByServerRelativeUrl(listName);
            FileCreationInformation fci = new FileCreationInformation();
            fci.Content = System.IO.File.ReadAllBytes("SampleFile.txt");

            fci.Url = "Item added by Job at " + DateTime.Now.Minute + " " + DateTime.Now.Second;
            fci.Overwrite = true;


            Microsoft.SharePoint.Client.File upFile = myList.RootFolder.Files.Add(fci);

            clientContext.Load(upFile);
            clientContext.ExecuteQuery();
        }

    }

        class AccessTokenResponse
    {
        public string token_type { get; set; }
        public string expires_in { get; set; }
        public string expires_on { get; set; }
        public string not_before { get; set; }
        public string resource { get; set; }
        public string access_token { get; set; }
        public string refresh_token { get; set; }
        public string scope { get; set; }
        public string id_token { get; set; }
        public string pwd_exp { get; set; }
        public string pwd_url { get; set; }
    }

        class RegisterHelper
        {
            public static bool HasValue(Microsoft.Win32.RegistryKey rootKey, string subKey)
            {
                try
                {
                    Microsoft.Win32.RegistryKey key = rootKey.OpenSubKey(subKey);
                    if (null == key || null == key.GetValueNames())
                        return false;
                    return true;
                }
                catch (Exception)
                {
                    return false;
                }
            }

            public static void WriteToRegister(Microsoft.Win32.RegistryKey rootKey, string subKey, Dictionary<string, string> nameAndValue)
            {
                //save value to register
                try
                {
                    Microsoft.Win32.RegistryKey key = rootKey.CreateSubKey(subKey);

                    foreach (var dic in nameAndValue)
                    {
                        key.SetValue(dic.Key, dic.Value);
                        Console.WriteLine("RegisterKey: {0}\nName: {1}\nValue: {2}\n\n", key.Name, dic.Key, dic.Value);
                    }

                }
                catch (Exception)
                {

                }
            }

            public static Dictionary<string, string> ReadFromRegister(Microsoft.Win32.RegistryKey rootKey, string subKey)
            {
                try
                {
                    Dictionary<string, string> dic = new Dictionary<string, string>();
                    Microsoft.Win32.RegistryKey key = rootKey.OpenSubKey(subKey);
                    if (null == key)
                    {
                        string error = string.Format("Open Key: [{0}] failed", key.Name);
                        throw new Exception(error);
                    }

                    string[] names = key.GetValueNames();
                    foreach (string name in names)
                    {
                        var value = key.GetValue(name);
                        if (null == value)
                        {
                            string error = string.Format("Name: {0} has no value!!", name);
                            throw new Exception(error);
                        }
                        dic.Add(name, value as string);
                    }
                    return dic;
                }
                catch (Exception)
                {
                    return null;
                }
            }
        }
}
