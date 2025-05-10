using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using OpenCvSharp
using OpenCvSharp.CPlusPlus;

namespace cs_test
{
    internal class Program
    {
        public struct ST_ActiveTemplateName
        {
            public string data;
        };


        public const string cppDLL = "cs_jetdll.dll";

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr cs_Init();

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void cs_Close(IntPtr p_Handle);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int cs_read_shape_directory(IntPtr p_Handle, string p_directory, string p_extension);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int cs_set_templates_to_detect(IntPtr p_Handle, ST_ActiveTemplateName[] p_activeTemplateNames, int p_nItemCount);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int cs_set_detector_params(IntPtr p_Handle, int MaxNumMatches, double ObjPixRatioThrMin, double ObjPixRatioThrMax, int MaxAngleDeviation, int AngleSearchStep, double ScaleSearchMin, double ScaleSearchMax, double ScaleSearchStep);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr cs_new_image(string p_imgPath);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void cs_del_image(IntPtr p_MatImg);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int cs_detect_shape(IntPtr p_Handle, IntPtr p_MatImg);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int cs_locate_shape(IntPtr p_Handle, IntPtr p_MatImg);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int cs_gnumTemplateFound(IntPtr p_Handle);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern string cs_getbestMatchName(IntPtr p_Handle, int p_nID);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern string cs_getbestMatchCorr(IntPtr p_Handle, int p_nID);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern string cs_getbestMatchScale(IntPtr p_Handle, int p_nID);

        [DllImport(cppDLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern string cs_getbestMatchAngle(IntPtr p_Handle, int p_nID);


        static void Main(string[] args)
        {
            IntPtr w_Handle = cs_Init();
            try
            {
                string w_directory = "ince_tasarimlar";
                string w_extension = ".png";
                int resRead = cs_read_shape_directory(w_Handle, w_directory, w_extension);
                if (resRead == -1)
                {
                    Console.WriteLine("Template folder couldn't be read.");
                    return;
                }
                else
                {
                    Console.WriteLine("Template folder has been read.");
                }

                string w_activeTemplateNames = "all";
                int w_nItemCount = 1;
                ST_ActiveTemplateName[] w_ActiveTemplateNames = new ST_ActiveTemplateName[w_nItemCount];
                w_ActiveTemplateNames[0].data = w_activeTemplateNames;
                int resTempRead = cs_set_templates_to_detect(w_Handle, w_ActiveTemplateNames, w_nItemCount);
                if (resTempRead == -1)
                {
                    Console.WriteLine("activeTemplates.txt file couldn't be created!");
                    return;
                }
                else
                {
                    Console.WriteLine("activeTemplates.txt file has been created!");
                }

                //call this to change detector params when necessary
                cs_set_detector_params(w_Handle, 10, 0.001, 0.5, 9, 3, 0.5, 0.6, 0.03);


                string w_ImgPath = "image_2023-11-24_14-33-57.png";
                IntPtr w_MatImg = cs_new_image(w_ImgPath);

                //. 
                int resLocate = cs_locate_shape(w_Handle, w_MatImg);
                //. 
                int resDetect = cs_detect_shape(w_Handle, w_MatImg);
                cs_del_image(w_MatImg);
                if (resDetect == -1)
                {
                    Console.WriteLine("error detecting the shape!");
                    return;
                }

                int w_numTemplateFound = cs_gnumTemplateFound(w_Handle);
                Console.WriteLine($"matched with {w_numTemplateFound}template images");
                for (int i = 0; i < w_numTemplateFound; i++)
                {
                    string bestMatchName = cs_getbestMatchName(w_Handle, i);
                    string bestMatchCorr = cs_getbestMatchCorr(w_Handle, i);
                    string bestMatchScale = cs_getbestMatchScale(w_Handle, i);
                    string bestMatchAngle = cs_getbestMatchAngle(w_Handle, i);

                    Console.WriteLine($"{bestMatchName} detected with confidence: {bestMatchCorr} , Angle: {bestMatchAngle} , Scale : {bestMatchScale}");

                }
            }
            finally
            {
                //.
                cs_Close(w_Handle);
            }


        }
    }
}
