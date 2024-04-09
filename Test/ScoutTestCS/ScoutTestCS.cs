using System;
using System.Runtime.InteropServices;
using System.Text;
//using ScoutUtilities.Structs;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

public enum HawkeyeError
{
    eSuccess = 0,
    eErrorUnknown,
    eInvalidArgs,
    eNotPermittedByUser,
    eNotPermittedAtThisTime,
    eAlreadyExists,
    eBusy,
    eTimedout,
    eHardwareFault,
    eValidationFailed,
    eInvalidPath,
    eFileNotFound,
    eEntryNotFound,
    eRunning,
    ePaused,
    eStopped,
};

public struct SamplePosition
{
    char row;
    byte column;

    bool isValid()
    {
        return ((row >= 'A' && row <= 'H') && (column >= 1 && column <= 12)) ||
               ((row == 'Z') && (column >= 1 && column <= 24));
    }
};

public enum eSampleStatus : ushort
{
    eNotProcessed = 0,
    eInProcess_1,   // 5 states of "in process"
    eInProcess_2,
    eInProcess_3,
    eInProcess_4,
    eInProcess_5,
    eCompleted,
    eSkip_Manual,
    eSkip_Error,
};

public enum eSamplePostWash : ushort
{
    eMinWash = 0,
    eMaxWash,
};

[StructLayout(LayoutKind.Sequential)]
public struct WorkQueueItem
{
    public string label;
    public string comment;
    public SamplePosition location;
    public uint celltypeIndex;
    public uint batchIndex;
    public uint dilutionFactor;
    public eSamplePostWash postWash;
    public string bp_qc_name;
    public byte numAnalyses;
    public uint[] analysisIndices;
    public eSampleStatus status;
}

[StructLayout(LayoutKind.Sequential)]
public struct WorkQueueItemFromDLL
{
    [MarshalAs(UnmanagedType.LPStr)]
    public string label;
    [MarshalAs(UnmanagedType.LPStr)]
    public string comment;
    public SamplePosition location;
    public uint celltypeIndex;
    public uint batchIndex;
    public uint dilutionFactor;
    public eSamplePostWash postWash;
    [MarshalAs(UnmanagedType.LPStr)]
    public string bp_qc_name;
    public byte numAnalyses;
    public IntPtr analysisIndices;
    public eSampleStatus status;
}

public enum eCellDeclusterSetting : ushort
{
    eDCNone = 0,// Declustering disabled.
    eDCLow,     // Wider spacing between potential cell centers
                // higher edge threshold
                // higher accumulator (less aggressive, more missed cells)

    eDCMedium,  // Middle of the road

    eDCHigh,    // Narrower spacing between potential cell centers
                // lower edge threshold
                // lower accumulator (more aggressive, more false positives)
};

[StructLayout(LayoutKind.Sequential)]
public struct CellType
{
    public uint celltype_index;
    public IntPtr label;
    public UInt16 max_image_count;
    public float minimum_diameter_um;
    public float maximum_diameter_um;
    public float minimum_circularity;
    public float sharpness_limit;
    public uint num_cell_identification_parameters;
    public IntPtr cell_identification_parameters;
    public eCellDeclusterSetting decluster_setting;
    public float fl_roi_extent;
    public IntPtr num_analysis_specializations;
    public IntPtr analysis_specializations;
}

[StructLayout(LayoutKind.Sequential)]
struct CallbackStruct
{
    uint a;
    uint b;
};

public enum eCarrierType : ushort
{
    eCarousel = 0,
    ePlate_96,
};

public enum ePrecession : ushort
{
    eRowMajor = 0,
    eColumnMajor,
};

[StructLayout(LayoutKind.Sequential)]
public struct WorkQueue
{
    public uuid   wlUuid;
    public IntPtr userId;
    public IntPtr label;
    public ulong  timestamp;
    public uuid   curSampleUuid;
    public eCarrierType carrier;
    public ePrecession precession;
    public WorkQueueItem defaultParameterSettings;
    public ushort numSampleSets;
    public IntPtr sampleSets;
};

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct AnalysisDefinition
{
    public ushort Analysis_index;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 50)]
    public string label;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 10)]
    public string result_unit;

    public byte num_reagents;
    public IntPtr reagentIndices;

    public byte num_fl_illuminators;
    public IntPtr fl_illuminators;

    public byte num_parameters;
    public IntPtr analysis_parameters;
    public bool ContainsFluorescence() { return num_fl_illuminators > 0; }
    public bool IsBCIAnalysis() {
        if ((Analysis_index & 0x80000000) == 0) {
            return false;
        } else {
            return true;
        }
    }
    public bool IsCustomerAnalysis() { return !IsBCIAnalysis(); }
};

public enum UserPermissionLevel
{
    eNormal = 0,
    eElevated,
    eAdministrator,
    eService,
};

[StructLayout(LayoutKind.Sequential)]
public unsafe struct uuidDLL {
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
    byte[] u;
};

public unsafe struct uuid
{
    fixed byte u[16];
};

public struct imagewrapper
{
    ushort rows;
    ushort cols;
    byte type; // OpenCV type ex: CV_8UC1 == 0 (see "types_c.h" from OpenCV
    ulong step; // bytes per row, including padding
    IntPtr data; // Data buffer.  For multi-channel, will be in BGR order
};

public struct fl_imagewrapper
{
    ushort fl_channel;
    imagewrapper fl_image;

};

public struct imagesetwrapper
{
    imagewrapper brightfield_image;

    byte num_fl_channels;
    IntPtr fl_images;
};

public struct BasicResultAnswers
{
    //Basic answers (pop count, pop of interest count, concentrations, some averages...)

    /// PRIMARY
    ulong count_pop_general;
    ulong count_pop_ofinterest;
    float concentration_general;    // x10^6
    float concentration_ofinterest; // x10^6
    float percent_pop_ofinterest;

    /// SECONDARY
    float avg_diameter_pop;
    float avg_diameter_ofinterest;
    float avg_circularity_pop;
    float avg_circularity_ofinterest;

    /// TERTIARY
    float coefficient_variance;
    ushort average_cells_per_image;
    ushort average_brightfield_bg_intensity;
    ushort bubble_count;
    ushort large_cluster_count;
};




namespace ScoutTestCS
{
    class ScoutTestCS
    {
        public enum NativeDataType
        {
            eChar,
            eUint16,
            eInt16,
            eUint32,
            eInt32,
            eUint64,
            eInt64
        }

        /// <summary>
        /// Frees a tagged native data type (which may be an array).
        /// </summary>
        /// <param name="tag">The type of data instance to be freed.</param>
        /// <param name="ptr">The pointer to data to be freeed.</param>
        /// <param name="number of items in the list">The number of items in the list.</param>
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError FreeTaggedBuffer(NativeDataType tag, IntPtr ptr);

        /// <summary>
        /// Frees a list (represented as an array) of tagged native data types.
        /// </summary>
        /// <param name="tag">The type of data instances to be freed.</param>
        /// <param name="ptr">The pointer to data to be freeed.</param>
        /// <param name="numitems">The number of items in the list.</param>
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError FreeListOfTaggedBuffers(NativeDataType tag, IntPtr ptr, UInt32 numitems);

        /// <summary>
        /// Frees a character buffer (array).
        /// </summary>
        /// <param name="ptr">The pointer to data to be freeed.</param>
        /// <returns>HawkeyeError.</returns>
        [DllImport("HawkeyeCore.dll")]
        public static extern void FreeCharBuffer(IntPtr ptr);

        /// <summary>
        /// Frees a list (represented as an array) of character buffers (arrays).
        /// </summary>
        /// <param name="ptr">The pointer to data to be freeed.</param>
        /// <param name="numitems">The number of items in the list.</param>
        [DllImport("HawkeyeCore.dll")]
        public static extern void FreeListOfCharBuffers(IntPtr ptr, UInt32 numitems);

        [DllImport("HawkeyeCore.dll")]
        public static extern void Initialize(bool without_hardware = false);
        [DllImport("HawkeyeCore.dll")]
        public static extern bool IsInitializationComplete();
        [DllImport("HawkeyeCore.dll", CharSet = CharSet.Ansi)]
        public static extern HawkeyeError LoginUser(string username, string password);
        [DllImport("HawkeyeCore.dll", CharSet = CharSet.Ansi)]
        //public static extern HawkeyeError GetUserCellTypeIndices(string username, out uint nCells, out IntPtr user_cell_indices);
        static extern HawkeyeError GetUserCellTypeIndices(string userName, out UInt32 nCells, out NativeDataType tag, out IntPtr ptrUserCellTypes);
        [DllImport("HawkeyeCore.dll", CharSet = CharSet.Ansi)]
        //public static extern HawkeyeError GetMyCellTypeIndices(out uint nCells, out IntPtr user_cell_indices);
        static extern HawkeyeError GetMyCellTypeIndices(out UInt32 nCells, out NativeDataType tag, out IntPtr cellTypeIndex);
        [DllImport("HawkeyeCore.dll", CharSet = CharSet.Ansi)]
        public static extern HawkeyeError GetMyDisplayName(out IntPtr displayname);
        [DllImport("HawkeyeCore.dll", CharSet = CharSet.Ansi)]
        public static extern HawkeyeError GetCurrentUser (out IntPtr userName, out int permissionLevel);
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError FreeWorkQueueItem(IntPtr wqi, uint queueLen);
        [DllImport("HawkeyeCore.dll", CharSet = CharSet.Ansi)]
        public static extern HawkeyeError GetUserList(bool onlyEnabled, out IntPtr userList, out uint numUsers);
        [DllImport("HawkeyeCore.dll", CharSet = CharSet.Ansi)]
        public static extern HawkeyeError GetMyCellTypes(out uint nCellTypes, out IntPtr celltypes);
        [DllImport("HawkeyeCore.dll", CharSet = CharSet.Ansi)]
        public static extern HawkeyeError GetAllCellTypes(out uint nCells, out IntPtr user_cell_indices);
        [DllImport("HawkeyeCore.dll", CharSet = CharSet.Ansi)]
        public static extern HawkeyeError GetFactoryCellTypes(out uint nCells, out IntPtr user_cell_indices);
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError FreeCharBuffer(ref IntPtr ptr);
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError FreeArrayOfCharBuffer(IntPtr ptr, uint numitems);
        [DllImport("HawkeyeCore.dll")]
        public static extern void FreeCellType(IntPtr list, uint num_celltypes);
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError SetWorklist (Worklist wl);

        //[DllImport("HawkeyeCore.dll")]
        //public static extern HawkeyeError RetrieveWorkQueue(UUID id, out IntPtr rec);

        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError RetrieveWorkQueues(UInt64 start, UInt64 end, string userId, out IntPtr reclist, out uint listSize);
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError AddItemToWorkQueue(ref WorkQueueItem wqi);
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError GetAnalysisForCellType(uint ad_index, uint ct_index, out IntPtr ptrAnalysisDefinition);
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError FreeAnalysisDefinitions (IntPtr analysisDefinitions, uint numDefinitions);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void workqueue_status_callback(IntPtr ptr, uuidDLL uuid);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void workqueue_completion_callback (uuidDLL uuid);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void workqueue_image_result_callback (IntPtr ptr, ushort imageSeqNum, imagesetwrapper image, BasicResultAnswers cumulativeResults, BasicResultAnswers imageResults);
        [DllImport("HawkeyeCore.dll")]
        public static extern HawkeyeError StartWorkQueue(
            [MarshalAs(UnmanagedType.FunctionPtr)] workqueue_status_callback onWQIStart,
            [MarshalAs(UnmanagedType.FunctionPtr)] workqueue_status_callback onWQIComplete,
            [MarshalAs(UnmanagedType.FunctionPtr)] workqueue_completion_callback onWQComplete,
            [MarshalAs(UnmanagedType.FunctionPtr)] workqueue_image_result_callback onWQIImageProcessed
        );

        static void onWQIStart(IntPtr intPtr, uuidDLL uuid)
        {
            Console.WriteLine("workqueue_status_callback - onWQIStart");
            WorkQueueItemFromDLL wqi = (WorkQueueItemFromDLL)Marshal.PtrToStructure(intPtr, typeof(WorkQueueItemFromDLL));
            FreeWorkQueueItem (intPtr, 1);
        }

        static void onWQIComplete(IntPtr intPtr, uuidDLL uuid)
        {
            Console.WriteLine("workqueue_status_callback - onWQIComplete");
            WorkQueueItemFromDLL wqi = (WorkQueueItemFromDLL)Marshal.PtrToStructure(intPtr, typeof(WorkQueueItemFromDLL));
            FreeWorkQueueItem(intPtr, 1);
        }

        static void onWQComplete (uuidDLL uuid)
        {
            Console.WriteLine("workqueue_completion_callback - onWQComplete");
        }

        static void onWQIImageProcessed (IntPtr ptr, ushort imageSeqNum, imagesetwrapper image, BasicResultAnswers cumulativeResults, BasicResultAnswers imageResults)
        {
            Console.WriteLine("WorkqueueImage_result_callback - onWQIImageProcessed");
        }


        //*********************************************************************
        static void Main(string[] args)
        {
            Task.Run (() =>
            {
                Initialize(false);

                while (!IsInitializationComplete());
                string username = "bci_service";
                string password = "916348";
                HawkeyeError he = LoginUser(username, password);

                IntPtr intPtr;

                he = GetMyDisplayName(out intPtr);
                string displayname = Marshal.PtrToStringAnsi(intPtr);
                //FreeCharBuffer (ref intPtr);
                Console.Write("Display Name: ");
                Console.WriteLine(displayname);


                int iUserlevel;
                he = GetCurrentUser(out intPtr, out iUserlevel);
                string currentUsername = Marshal.PtrToStringAnsi(intPtr);
                UserPermissionLevel userPermissionLevel = (UserPermissionLevel)iUserlevel;
                //FreeCharBuffer(ref intPtr);
                Console.Write("Current Username: ");
                Console.WriteLine(currentUsername);


                // *GetUserCellTypes* requires admin enabled account.
                // This should return an error.
                uint nCells = 0;
                int[] user_cell_indices;
                int user_cell_index=0;
                NativeDataType tag;
                he = GetUserCellTypeIndices (username, out nCells, out tag, out intPtr);
                if (nCells > 0)
                {
                    user_cell_indices = new int[nCells];
                    Marshal.Copy (intPtr, user_cell_indices, 0, (int)nCells);
                    //FreeTaggedBuffer (tag, intPtr);
                }


                nCells = 0;
                he = GetMyCellTypeIndices (out nCells, out tag, out intPtr);
                if (nCells > 0)
                {
                    Console.WriteLine("My Cell Type Indices:");

                    user_cell_indices = new int[nCells];
                    Marshal.Copy(intPtr, user_cell_indices, 0, (int)nCells);
                    //FreeTaggedBuffer(tag, intPtr);

                    for (int i = 0; i < nCells; i++)
                    {
                        Console.WriteLine(user_cell_indices[i]);
                        user_cell_index = user_cell_indices[i];
                    }
                }


                nCells = 0;
                List<CellType> list = new List<CellType>();
                he = GetAllCellTypes(out nCells, out intPtr);
                if (nCells > 0)
                {
                    Console.WriteLine("All Cell Types:");

                    var cellTypeSize = Marshal.SizeOf(typeof(CellType));

                    for (int i = 0; i < nCells; i++)
                    {
                        CellType cellType = (CellType)Marshal.PtrToStructure(intPtr, typeof(CellType));
                        list.Add(cellType);
                        intPtr = new IntPtr(intPtr.ToInt64() + cellTypeSize);
                    }
                }


                nCells = 0;
                list = new List<CellType>();
                he = GetFactoryCellTypes(out nCells, out intPtr);
                if (nCells > 0)
                {
                    Console.WriteLine("Factory Cell Types:");

                    Int64 cellTypeSize = Marshal.SizeOf(typeof(CellType));

                    for (int i = 0; i < nCells; i++)
                    {
                        CellType cellType = (CellType)Marshal.PtrToStructure(intPtr, typeof(CellType));
                        list.Add(cellType);
                        intPtr = new IntPtr(intPtr.ToInt64() + cellTypeSize);
                    }
                }


                bool onlyEnabled = false;
                uint numUsers = 0;
                he = GetUserList(onlyEnabled, out intPtr, out numUsers);
                if (numUsers > 0)
                {
                    IntPtr[] pIntPtrArray = new IntPtr[numUsers];
                    Marshal.Copy (intPtr, pIntPtrArray, 0, (int)numUsers);

                    Console.WriteLine("Known Users:");
                    string[] userList = new string[numUsers];
                    for (int i = 0; i < numUsers; i++)
                    {
                        userList[i] = Marshal.PtrToStringAnsi(pIntPtrArray[i]);
                        Console.WriteLine(userList[i]);
                    }
                    //FreeArrayOfCharBuffer (intPtr, numUsers);
                }

                Console.WriteLine("Building work queue");



                WorkQueue wq = new WorkQueue();
                wq.numWQI = 2;
                wq.curWQIIndex = 1;
                wq.carrier = eCarrierType.eCarousel;
                wq.precession = ePrecession.eRowMajor;
                wq.workQueueItems = new IntPtr();
                wq.additionalWorkSettings = new WorkQueueItem();

                WorkQueueItem[] wqitems = new WorkQueueItem[wq.numWQI];

                //calling AddItemToWorkQueue 
                WorkQueueItem wqitem = new WorkQueueItem();
                wqitem.label = "Hole 1";
                wqitem.comment = "Comment 1";
                wqitem.batchIndex = 42;
                wqitem.dilutionFactor = 5;
                wqitem.postWash = eSamplePostWash.eMinWash;
                wqitem.bp_qc_name = "";
                wqitem.numAnalyses = 1;

                wqitem.analysisIndices = new uint[1];
                wqitem.analysisIndices[0] = 1;

                wqitem.celltypeIndex = (uint)user_cell_index;
                wqitem.status = eSampleStatus.eNotProcessed;

                wqitems[0] = wqitem;

                wqitem = new WorkQueueItem();
                wqitem.label = "Hole 2";
                wqitem.comment = "Comment 2";
                wqitem.batchIndex = 0;
                wqitem.dilutionFactor = 5;
                wqitem.postWash = eSamplePostWash.eMinWash;
                wqitem.bp_qc_name = "";
                wqitem.numAnalyses = 1;
                wqitem.analysisIndices = new uint[1];
                wqitem.analysisIndices[0] = 2;
                wqitem.status = eSampleStatus.eNotProcessed;
                wqitem.celltypeIndex = (uint)user_cell_index;

                wqitems[1] = wqitem;

                var wqiSize = Marshal.SizeOf(typeof(WorkQueueItem));

                IntPtr pWQ = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(WorkQueue)));
                wq.workQueueItems = Marshal.AllocCoTaskMem(wq.numWQI * Marshal.SizeOf(typeof(WorkQueueItem)));

                for (int i = 0; i < wq.numWQI; i++)
                {
                    IntPtr pTmp = wq.workQueueItems + (i * wqiSize);
                    Marshal.StructureToPtr(wqitems[i], pTmp, false);
                }

                Marshal.StructureToPtr(wq, pWQ, false);

                Console.WriteLine("Setting work queue");
                he = SetWorkQueue(wq);

                Console.WriteLine("Starting work queue");
                he = StartWorkQueue(onWQIStart, onWQIComplete, onWQComplete, onWQIImageProcessed);


                Console.WriteLine("Calling GetAnalysisForCellType...");

                uint adIndex = 1;
                uint ctIndex = 1;

                IntPtr ptrAnalysisDefinition;
                HawkeyeError hawkeyeError = GetAnalysisForCellType(adIndex, ctIndex, out ptrAnalysisDefinition);
                AnalysisDefinition analysisDefinition = (AnalysisDefinition)Marshal.PtrToStructure (ptrAnalysisDefinition, typeof(AnalysisDefinition));
                FreeAnalysisDefinitions (ptrAnalysisDefinition, 1);

                Console.WriteLine("returning from GetAnalysisForCellType...");

            }).GetAwaiter().GetResult();

            ConsoleKeyInfo key = Console.ReadKey();

            Console.WriteLine ("exiting Main...");
        }
    }
}

