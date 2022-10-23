using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Win32;

namespace Nextlabs.Solution.Helpers
{
    public class RegistryKeyHelper
    {
        public static readonly RegistryKey rootKey_CurrentUser  = Registry.CurrentUser;

        public static readonly RegistryKey rootKey_LocalMachine = Registry.LocalMachine;

        public static readonly string subKey_RMS                = @"SOFTWARE\Nextlabs\RMS";

        public static readonly string subKey_SPOE               = @"SOFTWARE\Nextlabs\SPOE";

        public static readonly string subKey_AAD                = @"SOFTWARE\Nextlabs\Office365";

        public static readonly string subKey_InstallInfo        = @"SOFTWARE\Nextlabs\InstallInfo";

        public static bool HasValue(RegistryKey rootKey, string subKey)
        {
            try
            {
                RegistryKey key = rootKey.OpenSubKey(subKey);
                if (null == key || null == key.GetValueNames())
                    return false;
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        public static void WriteToRegister(RegistryKey rootKey, string subKey, Dictionary<string, string> nameAndValue)
        {
            //save value to register
            try
            {
                RegistryKey key = rootKey.CreateSubKey(subKey);

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

        public static Dictionary<string, string> ReadFromRegister(RegistryKey rootKey, string subKey)
        {
            try
            {
                Dictionary<string, string> dic = new Dictionary<string, string>();
                RegistryKey key = rootKey.OpenSubKey(subKey);
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
