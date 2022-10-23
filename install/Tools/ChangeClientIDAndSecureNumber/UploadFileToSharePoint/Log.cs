using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;
using System.Threading;

namespace SPOEBatchWorker
{
    /**
    * 可以记录 自动记录 打印Log的函数名，文件名，行号
    * 可以根据需要决定是否写入文件
    * Log 类不抛出异常，不返回错误
    * Log 的开关应当可配置（是否输出DebugViewLog，是否写入Log文件）
    */
    public static class Log
    {
        // Controls
        private static bool m_bWriterLogInfoToFile = true;
        private static bool m_bPrintLogInfoToDebugView = true;

        // Configure
        private static string m_strLogCatalog = "SPOE";
        private static string m_strLogFileNameFlag = "BatchModuleNormalLog";
        private static int m_nLogFileMaxSize = 1024 * 1024 * 2; // 2M

        public static void OutputLogInfo(string strInfo, System.Diagnostics.StackFrame sfLog = null)  // sfLog = new System.Diagnostics.StackFrame(true);
        {
            try
            {
                string strFullLogInfo = strInfo;
                if (null != sfLog)
                {
                    strFullLogInfo = string.Format("{0}::{1}::{2}: \r\n\t{3}", sfLog.GetFileName(), sfLog.GetMethod().Name, sfLog.GetFileLineNumber(), strInfo);
                }

                if (m_bPrintLogInfoToDebugView)
                {
                    PrintLog(strFullLogInfo);
                }

                if (m_bWriterLogInfoToFile)
                {
                    WriteLogFile(strFullLogInfo, m_strLogFileNameFlag);
                }
            }
            catch (System.Exception ex)
            {
                PrintLog("Exception in Outputloginfo:" + ex.Message + "\r\n;" + ex.StackTrace);
            }
        }

        public static void WriteLogFile(string strInfo, string strFileNameFlag)
        {
            // Get log Directory and file name
            string strDir = System.IO.Directory.GetCurrentDirectory() + "\\Log";
            if (!System.IO.Directory.Exists(strDir))
            {
                System.IO.Directory.CreateDirectory(strDir);
            }

            string strLogFileName = Thread.CurrentThread.ManagedThreadId.ToString() + "_" + strFileNameFlag + ".txt";
            string strLogFileFullName = strDir + "\\" + strLogFileName;

            FileInfo finfo = new FileInfo(strLogFileFullName);
            if (!finfo.Exists)
            {
                using (FileStream fs = System.IO.File.Create(strLogFileFullName))
                {
                    fs.Close();
                    finfo = new FileInfo(strLogFileFullName);
                }
            }
            else
            {
                // If the file large than m_nLogFileMaxSize, create a new one
                if (m_nLogFileMaxSize < finfo.Length)
                {
                    // Rename old log file and create new one
                    string strNewLogFileFullName = strDir + "\\" + System.Guid.NewGuid().ToString() + ".txt";
                    System.IO.File.Move(strLogFileFullName, strNewLogFileFullName);

                    // Create a new one
                    using (FileStream fs = System.IO.File.Create(strLogFileFullName))
                    {
                        fs.Close();
                        finfo = new FileInfo(strLogFileFullName);
                    }
                }
            }

            // write log
            using (FileStream fs = finfo.OpenWrite())
            {
                StreamWriter swWriteFile = new StreamWriter(fs);
                swWriteFile.BaseStream.Seek(0, SeekOrigin.End); // Add text at the end of file

                swWriteFile.WriteLine(strInfo);

                swWriteFile.Flush();
                swWriteFile.Close();
            }
        }

        private static void PrintLog(string strInfo)
        {
            Console.WriteLine(strInfo);
            Trace.WriteLine(strInfo, m_strLogCatalog);
        }
    }
}
