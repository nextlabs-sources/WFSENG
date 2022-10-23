/*
 * 更改Clinet ID 和 Client secure number
 * 更改web.config： service refrence address
 */

using System;
using System.IO;
using System.Xml;
using System.Configuration;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Compression;
using Nextlabs.Solution.Helpers;

namespace ChangeClientIDAndSecureNumber
{
    class Program
    {
        static void Main(string[] args)
        {
            try 
            {
                string strHostAppPath       = args[0].ToString();//"F:\\NLSPOLEventHandlerApp.app";
                string strProviderAppPath   = args[1].ToString();//"F:\\web";
                string appType              = args[2].ToString();//"spoe" or "rms";
                
                string strClientID                  = null;// "1234567";
                string secureNumber                 = null;// "7654321";
                string userEmail                    = null;// "BatchModule.SystemAccount@nextlabssolutions.onmicrosoft.com";
                string HostedAppHostNameOverride    = null;//"www.nextlabs.solutions";
                string value;
                string subKey;
                switch (appType.ToLower())
                {
                    case "spoe":
                        subKey = RegistryKeyHelper.subKey_SPOE;
                        break;
                    case "rms":
                        subKey = RegistryKeyHelper.subKey_RMS;
                        break;
                    default:
                        throw new Exception("not a valid parameter!");
                }
                Dictionary<string, string> SPOKeyValue = RegistryKeyHelper.ReadFromRegister(RegistryKeyHelper.rootKey_CurrentUser, subKey);
                foreach (string name in SPOKeyValue.Keys)
                {
                    switch (name.ToLower())
                    {
                        case "clientid":
                            if (SPOKeyValue.TryGetValue(name, out value))
                                strClientID = value;
                            else
                                strClientID = string.Empty;
                            break;
                        case"clientsecret":
                            if (SPOKeyValue.TryGetValue(name, out value))
                                secureNumber = value;
                            else
                                secureNumber = string.Empty;
                            break;
                        case"appdomain":
                            if (SPOKeyValue.TryGetValue(name, out value))
                                HostedAppHostNameOverride = value;
                            else
                                HostedAppHostNameOverride = string.Empty;
                            break;

                    }
                }
                //Dictionary<string, string> O365KeyValue = RegistryKeyHelper.ReadFromRegister(RegistryKeyHelper.rootKey_CurrentUser, RegistryKeyHelper.subKey_AAD);
                //foreach (string name in O365KeyValue.Keys)
                //{
                //    switch (name.ToLower())
                //    {
                //        case"useremail":
                //            if (O365KeyValue.TryGetValue(name, out value))
                //                userEmail = value;
                //            else
                //                userEmail = string.Empty;
                //            break;
                //    }
                //}
                
                if (strHostAppPath != null)
                {
                    var result = ChangeClientIDAndSecureNumber(strHostAppPath, strProviderAppPath, strClientID, secureNumber, /*userEmail,*/ HostedAppHostNameOverride);
                    Dictionary<string, string> dic = new Dictionary<string, string>();
                    switch (appType.ToLower())
                    {
                        case "spoe":
                            dic.Add("ChangeSPOEApp", "passed");
                            RegistryKeyHelper.WriteToRegister(RegistryKeyHelper.rootKey_CurrentUser, RegistryKeyHelper.subKey_InstallInfo, dic);
                            break;
                        case "rms":
                            dic.Add("ChangeRMSApp", "passed");
                            RegistryKeyHelper.WriteToRegister(RegistryKeyHelper.rootKey_CurrentUser, RegistryKeyHelper.subKey_InstallInfo, dic);
                            break;
                    }
                }
                else
                {
                    Console.WriteLine("Parameters cannot be empty");
                }
                //Console.ReadKey();

            }
            catch(Exception e)
            {
                Console.WriteLine(e.StackTrace + e.Source + e.Message + e.Data);
                Console.WriteLine("Parameters error ,please input correct parameters:\nParameter1 is the app path,for example:F:\\NLSPOLEventHandlerAp.app;\nParameter2 is event web install path,for example:F:\\Web;\nParameter3 is ClientID;\nParameter4 is secureNumber;\nParameter5 is HostedAppHostNameOverride;");
                Console.ReadKey();
            }
        }

        private static bool ChangeClientIDAndSecureNumber(string strHostAppPath, string strProviderAppPath, string strClientId, string strSecureNumber, /*string strUserEmail,*/ string HostedAppHostNameOverride)
        {
            bool result = false;
            try
            {
                #region 修改hostapp ClientID
               ZipClass zipclass = new ZipClass();
               if( File.Exists(strHostAppPath))
               {
                   string strHostAppAddZip = strHostAppPath + ".zip";//重命名为 .zip
                   if (!File.Exists(strHostAppAddZip))
                   {
                       File.Move(strHostAppPath, strHostAppAddZip);
                   }

                   string tempUnzipFilesDirectory = "C:\\tempFiles";//创建临时文件夹存放unzip
                   if(!Directory.Exists(tempUnzipFilesDirectory))
                   {
                       Directory.CreateDirectory(tempUnzipFilesDirectory);
                   }

                   zipclass.UnZip(strHostAppAddZip, tempUnzipFilesDirectory);//解压到临时文件中
                   //File.Delete(strHostAppAddZip);

                   string xmlFileName = tempUnzipFilesDirectory + "\\AppManifest.xml";
                   ReadXMLNodeForClientID(xmlFileName, strClientId);//, HostedAppHostNameOverride);//修改clientID
                   //Modify all the xml files in tempUnzipFilesDirectory, and change "~remoteAppUrl" to "https://" + HostedAppHostNameOverride + "/NLSPOERER"
                   ChangeRemoteAppUrl(tempUnzipFilesDirectory, HostedAppHostNameOverride);

                   //zipclass.ZipFileFromDirectory(tempUnzipFilesDirectory, strHostAppAddZip, 9);//重新压缩文件
                   //[2016-3-8] modify by luke, add every *.xml into zip file
                   DirectoryInfo tempDirectory = new DirectoryInfo(tempUnzipFilesDirectory);
                   foreach (FileInfo xmlFileTemp in tempDirectory.GetFiles("*.xml"))
                   {
                       AddfileToZip(xmlFileTemp.FullName, strHostAppAddZip);
                   }

                   DirectoryInfo di = new DirectoryInfo(tempUnzipFilesDirectory);
                   di.Delete(true);//删除临时文件夹

                   File.Move(strHostAppAddZip, strHostAppAddZip.Substring(0, strHostAppAddZip.LastIndexOf(".zip")));//重命名删掉zip
                   Console.WriteLine("complete!");
                #endregion
               }
               else
               {
                   Console.WriteLine("app does not exist!");
                   return false;
               }
               #region 修改providerapp ClientID和SecureNumber

               ChangeWebConfig(strProviderAppPath, strClientId, strSecureNumber, /*strUserEmail,*/ HostedAppHostNameOverride);//修改web.config的clientID和SecureNumber
               result = true;

               #endregion
            }
            catch(Exception ex) 
            {
                Console.WriteLine(ex.Message);
                return result;
            }

            return result;
        }

        private static void ReadXMLNodeForClientID(string xmlFileName, string ClientID)//, string HostedAppHostNameOverride)
        {
            XmlDocument xmlfile = new XmlDocument();
            try
            {
                xmlfile.Load(xmlFileName);
                var RemoteWebApplication = xmlfile.GetElementsByTagName("RemoteWebApplication");
                if(RemoteWebApplication != null)
                {
                    RemoteWebApplication[0].Attributes["ClientId"].InnerText = ClientID;
                    Console.WriteLine("Change ClientID success!");
                }
                //var Properties = xmlfile.GetElementsByTagName("Properties");
                //Properties[0].InnerXml = Properties[0].InnerXml.Replace("~remoteAppUrl", @"https://" + HostedAppHostNameOverride + "/NLSPOERER");
                xmlfile.Save(xmlFileName);
            }
            catch(Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        private static void ChangeRemoteAppUrl(string filesDirectory, string HostedAppHostNameOverride)
        {
            DirectoryInfo di = new DirectoryInfo(filesDirectory);
            foreach (FileInfo file in di.GetFiles("*.xml"))
            {
                StreamReader sReader = new StreamReader(file.FullName);
                string strFromFile = sReader.ReadToEnd();
                sReader.Close();
                strFromFile = strFromFile.Replace("~remoteAppUrl", @"https://" + HostedAppHostNameOverride + "/NLSPOERER");

                // use stream writer to overwrite the file
                StreamWriter sWriter = new StreamWriter(file.FullName, false);
                sWriter.Write(strFromFile);
                sWriter.Flush();
                sWriter.Close();
            }
        }

        private static void ChangeWebConfig(string strProviderAppPath, string ClientID, string SecureNumber, /*string UserEmail,*/ string HostedAppHostNameOverride)
        {
            try
            {
                string configpath = strProviderAppPath + "\\web.config";

                XmlDocument webconfig = new XmlDocument();

                webconfig.Load(configpath);

                if(webconfig != null)
                {

                XmlNode node = webconfig.DocumentElement.SelectSingleNode("appSettings");

                node.SelectSingleNode("descendant::add[@key='ClientId']").Attributes[1].Value = ClientID;
                node.SelectSingleNode("descendant::add[@key='ClientSecret']").Attributes[1].Value = SecureNumber;
                node.SelectSingleNode("descendant::add[@key='HostedAppHostNameOverride']").Attributes[1].Value = HostedAppHostNameOverride;
                //node.SelectSingleNode("descendant::add[@key='BatchModuleAccount']").Attributes[1].Value = UserEmail;
                webconfig.DocumentElement.SelectSingleNode("appSettings").InnerXml = node.InnerXml;
                webconfig.Save(configpath);

                Console.WriteLine("webconfig change success!");
                }
                else
                {
                    Console.WriteLine("providerAppPath error");
                }
            }
            catch(Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        private static void AddfileToZip(string strFilePath,string strZipPath)
        {
            try
            {
                using (FileStream strZipFile = new FileStream(strZipPath, FileMode.Open))
                {
                    using (ZipArchive archive = new ZipArchive(strZipFile, ZipArchiveMode.Update))
                    {
                        string filename = System.IO.Path.GetFileName(strFilePath);

                        if (archive.GetEntry(filename) != null)
                        {
                            ZipArchiveEntry deleteExistEntry = archive.GetEntry(filename);
                            deleteExistEntry.Delete();//文件存在先删除

                            ZipArchiveEntry readMeEntry = archive.CreateEntry(filename);
                            using (System.IO.Stream stream = readMeEntry.Open())
                            {
                                byte[] bytes = System.IO.File.ReadAllBytes(strFilePath);
                                stream.Write(bytes, 0, bytes.Length);//重新创建文件
                            }
                            Console.WriteLine("updata zip success!");
                        }
                        else
                        {
                            ZipArchiveEntry readMeEntry = archive.CreateEntry(filename);
                            using (System.IO.Stream stream = readMeEntry.Open())
                            {
                                byte[] bytes = System.IO.File.ReadAllBytes(strFilePath);
                                stream.Write(bytes, 0, bytes.Length);//重新创建文件
                            }
                            Console.WriteLine("updata zip success!");
                        }
                    }
                }
            }
            catch(Exception e)
            {
                Console.WriteLine("get some error :  " + e.Message);
            }
        }
        //bool ChangeWebServiceReference(string strFilePath, string strSerAddr)
        //{
        //    return true;
        //}
    }

}

