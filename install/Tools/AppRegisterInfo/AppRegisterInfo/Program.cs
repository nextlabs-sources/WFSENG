using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using Nextlabs.Solution.Helpers;
using System.Runtime.InteropServices;

namespace AppRegisterInfo
{
    class Program
    {
        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern int SetWindowPos(IntPtr hWnd, int hWndInsertAfter, int x, int y, int Width, int Height, int flags); 
        
        const string TitleId                = "ctl00_PlaceHolderMain_AppInfoSection_ctl02_TxtTitle";
        const string AppDomainId            = "ctl00_PlaceHolderMain_AppInfoSection_ctl03_TxtHostUri";
        const string RedirectUriId          = "ctl00_PlaceHolderMain_AppInfoSection_ctl04_TxtRedirectUri";
        static SHDocVw.InternetExplorer ie  = new SHDocVw.InternetExplorer();
        static mshtml.IHTMLDocument2 htmlDoc;

        static void Main(string[] args)
        {
            try
            {
                // use for write info into Regedit when user input info manually
                if (3 == args.Length)
                {
                    Dictionary<string, string> dic = new Dictionary<string, string>();
                    dic.Add("ClientId",     args[0]);
                    dic.Add("ClientSecret", args[1]);
                    dic.Add("AppDomain",    args[2]);
                    RegistryKeyHelper.WriteToRegister(RegistryKeyHelper.rootKey_CurrentUser, RegistryKeyHelper.subKey_SPOE, dic);
                    RegistryKeyHelper.WriteToRegister(RegistryKeyHelper.rootKey_CurrentUser, RegistryKeyHelper.subKey_RMS, dic);
                    Environment.Exit(0);
                }
                string sharepointFullUrl    = args[0].ToString(); //"https://nextlabssolutions.sharepoint.com/sites/AppCatalog;
                string innerHTML            = string.Empty;
                string appRegNewUrl         = string.Empty;
                string appInvUrl            = string.Empty;
                string SPOAppInfoPath       = System.Environment.GetEnvironmentVariable("ProgramData") 
                                            + @"\Nextlabs\SharePoint Online\datafile\RegisterInfo.txt";
                string RMSAppInfoPath       = System.Environment.GetEnvironmentVariable("ProgramData") 
                                            + @"\Nextlabs\RMS\datafile\spoe_setting.txt";

                //string xmlPath = @"c:\ProgramData\Nextlabs\SharePoint Online\AppRegisterInfo.xml";
                //string clientId, clientSecret, title, appDomain, redirectUri;

                if (sharepointFullUrl.EndsWith("/"))
                {
                    appRegNewUrl = sharepointFullUrl + "_layouts/15/appregnew.aspx";
                    appInvUrl    = sharepointFullUrl + "_layouts/15/appinv.aspx";
                }
                else
                {
                    appRegNewUrl = sharepointFullUrl + "/_layouts/15/appregnew.aspx";
                    appInvUrl    = sharepointFullUrl + "/_layouts/15/appinv.aspx";
                }

                ie.RegisterAsBrowser = true;
                ie.Visible           = true;
                ie.Navigate(appRegNewUrl);
                // set IE window to TopMost
                SetWindowPos((IntPtr)ie.HWND, -1, 0, 0, 0, 0, 1 | 2);
                // set IE window back to NoTopMost
                SetWindowPos((IntPtr)ie.HWND, -2, 0, 0, 0, 0, 1 | 2);

                // must use this loop first, otherwise it will throw a System.Runtime.InteropServices.COMException in WaitForResponse()
                while (ie.Busy)
                {
                    Thread.Sleep(500);
                }
                WaitForResponse();
                while (!ie.LocationURL.ToLower().Equals(appRegNewUrl.ToLower()))
                {
                    Console.WriteLine("Turning to {0} ...", appRegNewUrl);
                    htmlDoc   = ie.Document as mshtml.IHTMLDocument2;
                    if (htmlDoc.nameProp.Equals("Sign in to your account"))
                    {
                        Console.WriteLine("Please sign in to your account first...");
                        WaitForRequest();
                        WaitForResponse();
                    }
                }
                htmlDoc     = ie.Document as mshtml.IHTMLDocument2;
                innerHTML   = htmlDoc.body.innerHTML;

                RegexHelper regexHelper = new RegexHelper(innerHTML);


                mshtml.IHTMLElementCollection inputElementCollection = htmlDoc.all.tags("INPUT");
                Dictionary<string, string> idAndValue = new Dictionary<string,string>();
                idAndValue.Add(TitleId, "NextlabsApp");
                //idAndValue.Add(AppDomainId,);
                FillOut(inputElementCollection, idAndValue);
                while (!regexHelper.getAllInfoSuccess)
                {

                    Console.WriteLine("Can not get App Info yet...");
                    WaitForResponse();
                    htmlDoc = ie.Document as mshtml.IHTMLDocument2;
                    innerHTML = htmlDoc.body.innerHTML;
                    regexHelper = new RegexHelper(innerHTML);
                }
                if (regexHelper.getAllInfoSuccess)
                {
                    Dictionary<string, string> dic = new Dictionary<string,string>();
                    dic.Add("ClientId",     regexHelper.ClientId);
                    dic.Add("ClientSecret", regexHelper.ClientSecret);
                    dic.Add("Title",        regexHelper.Title);
                    dic.Add("AppDomain",    regexHelper.AppDomain);
                    dic.Add("RedirectUri",  regexHelper.RedirectURI);
                    RegistryKeyHelper.WriteToRegister(RegistryKeyHelper.rootKey_CurrentUser, RegistryKeyHelper.subKey_SPOE, dic);
                    RegistryKeyHelper.WriteToRegister(RegistryKeyHelper.rootKey_CurrentUser, RegistryKeyHelper.subKey_RMS,  dic);
                    //XmlHelper.writeToAnXml(System.Environment.GetEnvironmentVariable("ProgramData") + @"\Nextlabs\SharePoint Online\RegisterInfo.xml", regexHelper);
                    Directory.CreateDirectory(Path.GetDirectoryName(SPOAppInfoPath));
                    XmlHelper.writeToATxt(SPOAppInfoPath, regexHelper);
                    Microsoft.Win32.RegistryKey rootKeyForPath = Microsoft.Win32.Registry.LocalMachine;
                    Microsoft.Win32.RegistryKey key = rootKeyForPath.OpenSubKey(@"SOFTWARE\NextLabs,Inc.\RMS");

                    if (key != null)
                    {
                        string RMSDataDir = key.GetValue("DataDir") as string;
                        if (string.IsNullOrEmpty(RMSDataDir))
                        {
                            // write it to a fixed directory
                            Directory.CreateDirectory(Path.GetDirectoryName(RMSAppInfoPath));
                            XmlHelper.writeToATxt_RMS(RMSAppInfoPath, regexHelper, sharepointFullUrl);
                        }
                        else
                        {
                            Directory.CreateDirectory(RMSDataDir);
                            RMSAppInfoPath = RMSDataDir + "spoe_setting.txt";
                            XmlHelper.writeToATxt_RMS(RMSAppInfoPath, regexHelper, sharepointFullUrl);
                        }
                    }
                    #region add app permission
                    ie = new SHDocVw.InternetExplorer();
                    ie.Visible = true;
                    ie.Navigate(appInvUrl);
                    // set IE window to TopMost
                    SetWindowPos((IntPtr)ie.HWND, -1, 0, 0, 0, 0, 1 | 2);
                    // set IE window back to NoTopMost
                    SetWindowPos((IntPtr)ie.HWND, -2, 0, 0, 0, 0, 1 | 2);

                    
                    // must use this loop first, otherwise it will throw a System.Runtime.InteropServices.COMException in WaitForResponse()
                    while (ie.Busy)
                    {
                        Thread.Sleep(500);
                    }
                    WaitForResponse();
                    htmlDoc                                                 = ie.Document as mshtml.IHTMLDocument2;
                    inputElementCollection                                  = htmlDoc.all.tags("INPUT");
                    mshtml.IHTMLElementCollection textareaElementCollection = htmlDoc.all.tags("TEXTAREA");

                    idAndValue.Clear();
                    idAndValue.Add("ctl00_PlaceHolderMain_IdTitleEditableInputFormSection_ctl01_TxtAppId",       regexHelper.ClientId);
                    idAndValue.Add("ctl00_PlaceHolderMain_IdTitleEditableInputFormSection_ctl02_TxtTitle",       regexHelper.Title);
                    idAndValue.Add("ctl00_PlaceHolderMain_IdTitleEditableInputFormSection_ctl03_TxtRealm",       regexHelper.AppDomain);
                    idAndValue.Add("ctl00_PlaceHolderMain_IdTitleEditableInputFormSection_ctl04_TxtRedirectUrl", regexHelper.RedirectURI);
                    FillOut(inputElementCollection, idAndValue);

                    string appPermissions = "<AppPermissionRequests AllowAppOnlyPolicy=\"true\">" + Environment.NewLine
                                            + "\t<AppPermissionRequest Scope=\"http://sharepoint/content/sitecollection/web/list\" Right=\"FullControl\" />" + Environment.NewLine
                                            + "</AppPermissionRequests>";
                    idAndValue.Clear();
                    idAndValue.Add("ctl00_PlaceHolderMain_TitleDescSection_ctl01_TxtPerm", appPermissions);
                    FillOut(textareaElementCollection, idAndValue);

                    //mshtml.IHTMLElement create = inputElementCollection.item("ctl00_PlaceHolderMain_ctl01_RptControls_BtnCreate");
                    //create.click();
                    //WaitForRequest();
                    //WaitForResponse();

                    //htmlDoc                   = ie.Document as mshtml.IHTMLDocument2;
                    //inputElementCollection    = htmlDoc.all.tags("INPUT");
                    //mshtml.IHTMLElement trust = inputElementCollection.item("ctl00_PlaceHolderMain_BtnAllow");
                    //trust.click();
                    #endregion

                    //XmlHelper.setTmp(regexHelper);
                    MessageBox.Show("Register the app information success.\nYou can find the information file at '" + SPOAppInfoPath + "'.\n"
                                                                         + "                                     '" + RMSAppInfoPath + "'.\n\n\n"
                                                                         + "Now, please trust our App, just click [Create], then [Trust] it.", 
                                    "Register App Success!", 
                                    MessageBoxButtons.OK, 
                                    MessageBoxIcon.Information, 
                                    MessageBoxDefaultButton.Button1,
                                    MessageBoxOptions.DefaultDesktopOnly);
                    // set IE window to TopMost
                    SetWindowPos((IntPtr)ie.HWND, -1, 0, 0, 0, 0, 1 | 2);
                    // set IE window back to NoTopMost
                    SetWindowPos((IntPtr)ie.HWND, -2, 0, 0, 0, 0, 1 | 2);
                    Environment.Exit(0);
                }
            }
            catch (System.Runtime.InteropServices.COMException e)
            {
                Console.WriteLine("this IE window has been closed, can not get info any more");
                Console.WriteLine(e.StackTrace);
                MessageBox.Show("The page has been closed.\nIf you have not generating App Info, you can click the 'Regiter' button again in installer window.\n\nOr you can input the App Info manually.",
                                "The page has been closed",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Information,
                                MessageBoxDefaultButton.Button1,
                                MessageBoxOptions.DefaultDesktopOnly);
                Environment.Exit(0);
            }
            catch (Exception e)
            {
                string error = string.Format("StackTrack: {0}\nMessage: {1}\nSource: {2}\n\n", e.StackTrace, e.Message, e.Source);
                MessageBox.Show(error);
            }
        }

        static void WaitForResponse()
        {
            while (ie.Busy)
            {
                Thread.Sleep(500);
                htmlDoc = ie.Document as mshtml.IHTMLDocument2;
                if (htmlDoc.readyState.ToLower().Equals("complete"))
                    break;
            }
        }

        static void WaitForRequest()
        {
            while (!ie.Busy)
                Thread.Sleep(500);
        }

        static bool FillOut(mshtml.IHTMLElementCollection elementCollection, Dictionary<string, string> idAndValue)
        {
            try
            {
                foreach (string elementId in idAndValue.Keys)
                {
                    string value;
                    idAndValue.TryGetValue(elementId, out value);
                    mshtml.IHTMLElement element = elementCollection.item(elementId);
                    element.setAttribute("value", value);
                }
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }
    }
}
