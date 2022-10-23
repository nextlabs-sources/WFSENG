using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Nextlabs.Solution.Helpers
{
    public class ReadWriteFileHelper
    {
        public static void WriteToXml(string xmlPath, string[] nodeName, string[] innerText, bool hidden = false)
        {
            string startNode, endNode, contentText, singleLine;
            string allLines  = null;
            string xmlName   = Path.GetFileNameWithoutExtension(xmlPath);
            string directory = Path.GetDirectoryName(xmlPath);
            // make sure nodeName's count equals the innerText's count
            if(nodeName.Length != innerText.Length)
            {
                throw new Exception("Please make sure the nodeName and innerText are corresponded one to one!");
            }
            for (int i = 0; i < nodeName.Length; i++)
            {
                contentText = innerText[i];
                if (nodeName[i].StartsWith("</") && nodeName[i].EndsWith(">"))
                {
                    // remove the '/' like: </a> to <a>
                    startNode   = nodeName[i].Remove(1, 1);
                    endNode     = nodeName[i];
                }
                else if (nodeName[i].StartsWith("<") && nodeName[i].EndsWith(">"))
                {
                    startNode   = nodeName[i];
                    endNode     = nodeName[i].Insert(1, "/");
                }
                else
                {
                    startNode   = "<>".Insert(1, nodeName[i]);
                    endNode     = "</>".Insert(2, nodeName[i]);
                }
                singleLine  = "\t" 
                            + startNode 
                            + contentText 
                            + endNode 
                            + Environment.NewLine;
                allLines += singleLine;
            }

            // write to an xml file
            string finalXml = "<>".Insert(1, xmlName)
                            + allLines
                            + "</>".Insert(2, xmlName);

            if (!Directory.Exists(directory))
                Directory.CreateDirectory(directory);
            File.WriteAllText(xmlPath, allLines);
            if(hidden)
                File.SetAttributes(xmlPath, FileAttributes.Hidden);
        }

        public static void WriteTo(string fullPath, string contents, bool hidden = false)
        {
            string directory = Path.GetDirectoryName(fullPath);
            if (!Directory.Exists(directory))
                Directory.CreateDirectory(directory);
            File.WriteAllText(fullPath, contents);
            if (hidden)
                File.SetAttributes(fullPath, FileAttributes.Hidden);
        }
    }
}
