using System.Diagnostics;
using System.Xml.Serialization;

// FSharp.Data --version 4.2.8
using FSharp.Data;

// Plotly.NET --version 2.0.0-preview.18
using Plotly.NET;
using Plotly.NET.LayoutObjects;

class Application
{
    public NlpsoConfig Config = new NlpsoConfig();

    public Application(string[] args)
    {
        if (args[0].EndsWith(".xml"))
        {
            LoadXml(args[0]);
        }
        else if (args[0].EndsWith(".c"))
        {
            LoadCSource(args[0]);
        }
        else
        {
            throw new ArgumentException();
        }
    }

    private void LoadXml(string stream)
    {
        var serializer = new XmlSerializer(typeof(NlpsoConfig));
        try
        {
            using (var sr = new StreamReader(stream))
            {
                var c = serializer.Deserialize(sr);
                Config = (NlpsoConfig)(c ?? new NlpsoConfig());
                if (c == null) throw new ArgumentNullException();
            }
        }
        catch (ArgumentNullException)
        {
            PutError($"{stream} does not exists.");
            return;
        }
        catch (ArgumentException)
        {
            PutError($"{stream} does not support reading.");
            return;
        }
        catch (InvalidOperationException e)
        {
            PutError("An error occurred during deserialization:");
            Console.WriteLine(e);
            return;
        }
    }

    private void LoadCSource(string stream)
    {
        Config.Target = stream.Replace(".c", "");
        Config.Source = stream;

        string? line = "";
        using (var reader = new StreamReader(stream))
        {
            while ((line = reader.ReadLine()) != null)
            {
                if (line.StartsWith("const double LMAX[]"))
                {
                    int left = line.IndexOf('{');
                    int right = line.IndexOf('}');
                    Config.ParamUpperBound = line.Substring(left + 1, right - left - 1).Replace(" ", "").Split(',').Select(double.Parse).ToList();
                    Config.Dimension[0] = Config.ParamUpperBound.Count;
                }

                if (line.StartsWith("const double LMIN[]"))
                {
                    int left = line.IndexOf('{');
                    int right = line.IndexOf('}');
                    Config.ParamLowerBound = line.Substring(left + 1, right - left - 1).Replace(" ", "").Split(',').Select(double.Parse).ToList();
                    // Config.Dimension[0] = line.Substring(left + 1, right - left - 1).Replace(" ", "").Split(',').Length;
                }

                if (line.StartsWith("const double XMAX[]"))
                {
                    int left = line.IndexOf('{');
                    int right = line.IndexOf('}');
                    Config.StateUpperBound = line.Substring(left + 1, right - left - 1).Replace(" ", "").Split(',').Select(double.Parse).ToList();
                    Config.Dimension[1] = Config.StateUpperBound.Count;
                }

                if (line.StartsWith("const double XMIN[]"))
                {
                    int left = line.IndexOf('{');
                    int right = line.IndexOf('}');
                    Config.StateLowerBound = line.Substring(left + 1, right - left - 1).Replace(" ", "").Split(',').Select(double.Parse).ToList();
                    // Config.Dimension[1] = line.Substring(left + 1, right - left - 1).Replace(" ", "").Split(',').Length;
                }
            }
        }

        if (Config.Dimension[0] > 0 && Config.Dimension[1] > 0)
        {
            return;
        }

        throw new InvalidDataException();
    }

    public string Command(string input)
    {
        var line = input.Split();

        return line[0].ToLower() switch
        {
            // "help" => Help(),
            // "load" => Load(line),
            "save" => Save(),
            "saveas" => SaveAs(line),
            "set" => SetParam(line),
            "get" => GetParam(line),
            "run" => Run(),
            _ => "e_Could not execute because the specified command was not found."
        };
    }

    private string Load(string[] input)
    {
        if (input.Length < 2)
        {
            return "e_Please specify file name for load.";
        }

        string file = input[1];
        if (file.EndsWith(".xml"))
        {
            LoadXml(file);
            return "";
        }

        if (file.EndsWith(".c"))
        {
            LoadCSource(file);
            return "";
        }

        return "e_Could not load specified file. Only .xml files available.";
    }

    private string Save()
    {
        // todo: すべてのパラメータが有効でなければ保存しない
        // if (Config.Target == "")
        // {
        //     return "e_Before saving, specify the name of the target with >> set target ***.";
        // }

        try
        {
            Config.SaveXml();
        }
        catch (ArgumentNullException)
        {
            return $"e_Somthing went wrong with saving file.";
        }
        catch (ArgumentException)
        {
            return "e_Target stream is not writable.";
        }

        return "";
    }

    private string SaveAs(string[] input)
    {
        if (input.Length < 2)
        {
            return "e_Please specify file name for save.";
        }

        var filename = string.Join('_', input, 1, input.Length - 1);

        try
        {
            Config.SaveXml(input[1]);
        }
        catch (ArgumentNullException)
        {
            return $"e_Somthing went wrong with saving file.";
        }
        catch (ArgumentException)
        {
            return "e_Target stream is not writable.";
        }

        return "";
    }

    // private string SaveAs(string file)
    // {
    //     if (!file.EndsWith(".xml"))
    //     {
    //         file += ".xml";
    //     }

    //     var serializer = new XmlSerializer(type: typeof(NlpsoConfig));
    //     try
    //     {
    //         using (var sw = new StreamWriter(file, false))
    //         {
    //             serializer.Serialize(sw, Config);
    //         }
    //     }
    //     catch (ArgumentNullException)
    //     {
    //         return $"e_The stream \"{file}\" is null. The file may have failed to be generated.";
    //     }
    //     catch (ArgumentException)
    //     {
    //         return "e_{file} is not writable.";
    //     }

    //     return "";
    // }

    private string SetParam(string[] input)
    {
        if (input.Length < 3)
        {
            return "e_Specify the value to set.";
        }

        switch (input[1].ToLower())
        {
            case "target":
                Config.Target = string.Join(' ', input, 2, input.Length - 1);
                return "";

            case "trial":
                if (!int.TryParse(input[2], out int nt) || nt < 1)
                {
                    return "e_The swarm size must be positive integer.";
                }
                Config.NumTrial = nt;
                return "";

            case "period":
                var ls = new List<int>();
                for (int i = 2; i < input.Length; i++)
                {
                    if (!int.TryParse(input[i], out int p) || p < 1)
                    {
                        return "e_The number of period must be positive integer.";
                    }

                    ls.Add(p);
                }
                Config.Period = new List<int>(ls);
                return "";

            case "mu":
                if (!input[2].IsAnyOf("1", "-1"))
                {
                    return "e_The characteristic multiplier must be 1 or -1.";
                }
                Config.Mu = int.Parse(input[2]);
                return "";

            case "mbif":
                if (!int.TryParse(input[2], out int mb) || mb < 1)
                {
                    return "e_The swarm size must be positive integer.";
                }
                Config.Population[0] = mb;
                return "";

            case "mpp":
                if (!int.TryParse(input[2], out int mp) || mp < 1)
                {
                    return "e_The swarm size must be positive integer.";
                }
                Config.Population[1] = mp;
                return "";

            case "tbif":
                if (!int.TryParse(input[2], out int tb) || tb < 1)
                {
                    return "e_The limit of iteration count must be positive integer.";
                }
                Config.Tmax[0] = tb;
                return "";

            case "tpp":
                if (!int.TryParse(input[2], out int tp) || tp < 1)
                {
                    return "e_The limit of iteration count must be positive integer.";
                }
                Config.Tmax[1] = tp;
                return "";

            case "cbif":
                if (!double.TryParse(input[2], out double cb) || cb < 0)
                {
                    return "e_Stop criterion must be positive value.";
                }
                Config.Criterion[0] = cb;
                return "";

            case "cpp":
                if (!double.TryParse(input[2], out double cp) || cp < 0)
                {
                    return "e_Stop criterion must be positive value.";
                }
                Config.Criterion[1] = cp;
                return "";

            case "openmp":
                if (!int.TryParse(input[2], out int omp))
                {
                    return "e_The number of threads must be integer.";
                }

                if (omp < 2)
                {
                    Config.OpenMP = false;
                    return "";
                }

                Config.OpenMP = true;
                Config.Parallelism = omp;
                return "";

            default:
                return "e_Could not set because the specified parameter was not found.";
        }
    }

    private string GetParam(string[] input)
    {
        if (input.Length < 2)
        {
            Put($" Variable   |  key        |  value");
            Put($"------------+-------------+------------");
            return Config.ToString();
        }

        return Config.ToString(input[1]);
    }

    private string Run()
    {
        if (Config.Period.Count < 1)
        {
            return "e_Set the target period before starting the search.";
        }

        // save
        Config.SaveXml();

        // copy c source
        string dir = "csources";

        // これを踏むようならどのみち例外
        // Directory.CreateDirectory(dir);

        string source = $"{Config.Target}.c";
        string destin = $"{dir}/target.c";
        try
        {
            File.Copy(source, destin, true);
        }
        catch (UnauthorizedAccessException)
        {
            return "e_Failed to copy the source file due to lack of required permissions. Please consider changing permissions.";
        }
        catch (PathTooLongException)
        {
            return "e_File path exceed the system-defined maximum length.";
        }
        catch (FileNotFoundException)
        {
            return "e_Source file not found.";
        }

        // outディレクトリをクリア 無ければ作る
        DirectoryInfo outdir = Directory.CreateDirectory("out");
        FileInfo[] files = outdir.GetFiles();
        foreach (FileInfo file in files)
        {
            if (file.FullName.EndsWith(".csv"))
            {
                file.Delete();
            }
        }

        // 出力ファイル準備
        var periods = new HashSet<int>(Config.Period);
        foreach (int pd in periods)
        {
            using (var sw = new StreamWriter($"out/{(Config.Mu == -1 ? "I" : "G")}{pd}.csv", false))
            {
                sw.Write("t,");
                for (int d = 1; d <= Config.Dimension[0]; d++)
                {
                    sw.Write($"l{d},");
                }
                for (int d = 1; d <= Config.Dimension[1]; d++)
                {
                    sw.Write($"x{d},");
                }
                sw.WriteLine("Fbif");
            }

            using (var sw = new StreamWriter($"out/timeSpan_{(Config.Mu == -1 ? "I" : "G")}{pd}.csv", false))
            {
                sw.WriteLine("suc,time,[s],tbif");
            }
        }

        LinearAxis xAxis = LinearAxis.init<IConvertible, IConvertible, IConvertible, IConvertible, IConvertible, IConvertible>(
            Title: Title.init("&#955;<sub>1</sub>"),
            ZeroLineColor: Color.fromString("#ffff"),
            ZeroLineWidth: 2,
            GridColor: Color.fromString("#ffff"),
            Range: StyleParam.Range.ofMinMax(Config.ParamLowerBound[0], Config.ParamUpperBound[0])
        );

        LinearAxis yAxis = LinearAxis.init<IConvertible, IConvertible, IConvertible, IConvertible, IConvertible, IConvertible>(
            Title: Title.init("&#955;<sub>2</sub>"),
            ZeroLineColor: Color.fromString("#ffff"),
            ZeroLineWidth: 2,
            GridColor: Color.fromString("#ffff"),
            Range: StyleParam.Range.ofMinMax(Config.ParamLowerBound[1], Config.ParamUpperBound[1])
        );


        Layout layout = new Layout();
        layout.SetValue("title", Config.Target);
        layout.SetValue("xaxis", xAxis);
        layout.SetValue("yaxis", yAxis);
        layout.SetValue("showlegend", true);

        var scatterList = new List<GenericChart.GenericChart>();

        // var layout = Layout.init<IConvertible>(PlotBGColor: Color.fromString("#e5ecf6"));

        var startTime = DateTime.Now;
        int prev = -1;
        foreach (int pd in Config.Period)
        {
            if (pd != prev)
            {
                prev = pd;
                // generate .h
                try
                {
                    Config.GenerateHeader(pd);
                }
                catch (ArgumentException)
                {
                    return "e_Exception occured. csources/config.h is not writable.";
                }

                PutVerified("The header file was generated.");

                // compile
                if (!Compile(destin))
                {
                    return "e_Compilation failed.";
                }

                PutVerified("Compilation process successfully completed.");
            }

            // execute
            // todo: 失敗した場合の処理
            Console.WriteLine("Start NLPSO....");
            Execute(pd);
        }

        PutVerified("Search process finished.");
        Console.WriteLine("Start to generate diagrams...");

        foreach (int pd in periods)
        {
            // scatter
            string mu = Config.Mu == -1 ? "I" : "G";
            using (var csv = CsvFile.Load(Environment.CurrentDirectory + $"/out/{mu}{pd}.csv"))
            {
                IEnumerable<string> GetData(string column) => csv.Rows.Select(row => row.GetColumn(column));
                var x = GetData("l1").Select(double.Parse).ToArray();
                var y = GetData("l2").Select(double.Parse).ToArray();
                scatterList.Add(Chart2D.Chart.Scatter<double, double, string>(x, y, StyleParam.Mode.Markers, Name: $"{mu}{pd}").WithLayout(layout).WithMarkerStyle(Size: 3));
            }
        }

        var chart = Chart.Combine(scatterList);

        try
        {
            chart.SaveHtml("out/fig.html", true);
        }
        catch
        {
            if (File.Exists("out/fig.html") && File.GetLastWriteTime("out/fig.html").CompareTo(startTime) > 0)
            {
                return "w_No valid browser found. Failed to open out/fig.html .";
            }
        }

        return "done.";
    }

    private bool Compile(string sourcePath)
    {
        var options = new List<string>();
        options.Add("-lm");
        options.Add(Config.OptLevel switch
        {
            > 3 => "-Ofast",
            >= 0 => $"-O{Config.OptLevel.ToString()}",
            _ => ""
        });
        options.Add("-march=native");
        if (Config.OpenMP)
        {
            options.Add("-fopenmp");
        }
        options.Add("-w");
        options.Add("-o a.exe");

        var pInfo = new ProcessStartInfo()
        {
            FileName = "gcc",
            Arguments = "csources/nlpso.c csources/target.c csources/rand.c csources/watch.c " + string.Join(' ', options),
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
        };

        var process = Process.Start(pInfo);
        if (process == null)
        {
            return false;
        }

        process.WaitForExit();
        var output = process.StandardOutput.ReadToEnd();
        if (output.Length > 1)
        {
            Console.WriteLine(output);
            return false;
        }

        var error = process.StandardError.ReadToEnd();
        if (error.Length > 1)
        {
            Console.WriteLine(error);
            return false;
        }

        return true;
    }

    private bool Execute(int pd)
    {
        // Directory.CreateDirectory("out");
        var pInfo = new ProcessStartInfo()
        {
            FileName = "a.exe",
            Arguments = "",
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
        };

        // int suc = 0;
        // int fail = 0;

        using (var process = new Process())
        using (var ctoken = new CancellationTokenSource())
        {
            process.EnableRaisingEvents = true;
            process.StartInfo = pInfo;

            process.OutputDataReceived += (sender, ev) =>
            {
                Console.WriteLine(ev.Data);

                // todo:
                // if (ev.Data != null)
                // {
                //     var res = ev.Data.Split()[0];
                //     if (res == "I") suc++;
                //     if (res == "F") fail++;
                // }
                // var prog = (suc + fail) / (double)Config.NumTrial * 100;
                // Console.Write($"\r suc: {suc}  fail: {fail}  {prog.ToString("f1")}% complete.          ");
            };
            process.ErrorDataReceived += (sender, ev) =>
            {
                Console.WriteLine(ev.Data);
            };
            process.Exited += (sender, ev) =>
            {
                ctoken.Cancel();
            };

            process.Start();
            process.BeginErrorReadLine();
            process.BeginOutputReadLine();

            // wait for exit
            ctoken.Token.WaitHandle.WaitOne();
            process.WaitForExit();

            if (process.ExitCode == 0)
            {
                PutVerified($"The trial for {(Config.Mu == -1 ? 'I' : 'G')}{pd} successfully completed.");
                return true;
            }
            return false;
        }
    }

    public static void Put(string message)
    {
        Console.WriteLine(message);
        return;
    }

    public static void PutVerified(string message)
    {
        Console.ForegroundColor = ConsoleColor.Cyan;
        Console.Write("I: ");
        Console.ResetColor();
        Console.WriteLine(message);
        return;
    }

    public static void PutWarning(string message)
    {
        Console.ForegroundColor = ConsoleColor.Yellow;
        Console.Write("W: ");
        Console.ResetColor();
        Console.WriteLine(message);
        return;
    }

    public static void PutError(string message)
    {
        Console.ForegroundColor = ConsoleColor.Magenta;
        Console.Write("E: ");
        Console.ResetColor();
        Console.WriteLine(message);
        return;
    }
}