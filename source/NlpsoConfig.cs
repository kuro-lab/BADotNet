using System.Text;
using System.Xml.Serialization;

public class NlpsoConfig
{
    public string Target { get; set; } = "";
    public int NumTrial { get; set; } = 100;
    public List<int> Period { get; set; } = new();
    public List<int> Mu { get; set; } = new();
    public int[] Dimension { get; set; } = new int[] { 0, 0 };
    public int[] Population { get; set; } = new int[] { 30, 30 };
    public int[] Tmax { get; set; } = new int[] { 600, 300 };
    public double[] Criterion { get; set; } = new double[] { 1e-3, 1e-10 };
    public List<int> Parallelism { get; set; } = new();
    public bool OpenMP { get; set; } = true;
    public int OptLevel { get; set; } = 2;
    public string Source { get; set; } = "";
    public List<double> ParamUpperBound { get; set; } = new List<double>();
    public List<double> ParamLowerBound { get; set; } = new List<double>();
    public List<double> StateUpperBound { get; set; } = new List<double>();
    public List<double> StateLowerBound { get; set; } = new List<double>();

    public void SaveXml()
    {
        string file = Target + ".xml";
        SaveXml(file);
        return;
    }

    public void SaveXml(string file)
    {
        if (!file.EndsWith(".xml"))
        {
            file += ".xml";
        }
        var serializer = new XmlSerializer(type: typeof(NlpsoConfig));

        using (var sw = new StreamWriter(file, false))
        {
            serializer.Serialize(sw, this);
        }
        return;
    }

    public void GenerateHeader(int pd, int mu, int para)
    {
        var sb = new StringBuilder();
        sb.AppendLine($"#pragma once");
        sb.AppendLine($"#define PARALLELISM {para}");
        sb.AppendLine($"#define N {NumTrial}");
        sb.AppendLine($"#define PERIOD {pd}");
        sb.AppendLine($"#define MU {mu}");
        sb.AppendLine($"#define Dbif {Dimension[0]}");
        sb.AppendLine($"#define Dpp {Dimension[1]}");
        sb.AppendLine($"#define Mbif {Population[0]}");
        sb.AppendLine($"#define Mpp {Population[1]}");
        sb.AppendLine($"#define Tbif {Tmax[0]}");
        sb.AppendLine($"#define Tpp {Tmax[1]}");
        sb.AppendLine($"#define Cbif {Criterion[0]}");
        sb.AppendLine($"#define Cpp {Criterion[1]}");

        using (var writer = new StreamWriter("csources/config.h", false))
        {
            writer.WriteLine(sb.ToString());
        }

        return;
    }

    public override string ToString()
    {
        var sb = new StringBuilder();
        sb.AppendLine($" Target     |  target     | {Target}");
        sb.AppendLine($" NumTrial   |  trial      | {NumTrial}");
        sb.AppendLine($" Period     |  period     | {string.Join(", ", Period)}");
        sb.AppendLine($" Mu         |  mu         | {string.Join(", ", Mu)}");
        sb.AppendLine($" Population | (mbif, mpp) | ({string.Join(", ", Population)})");
        sb.AppendLine($" Tmax       | (tbif, tpp) | ({string.Join(", ", Tmax)})");
        sb.AppendLine($" Criterion  | (cbif, cpp) | ({string.Join(", ", Criterion)})");
        sb.AppendLine($" OpenMP     |  openmp     | {string.Join(", ", Parallelism)}");
        return sb.ToString();
    }

    public string ToString(string param)
    {
        return param.ToLower() switch
        {
            "target" => $"\"{Target}\"",
            "period" => $"({string.Join(", ", Period)})",
            "trial" => $"{NumTrial}",
            "mu" => $"{string.Join(", ", Mu)}",
            "population" => $"({string.Join(", ", Population)})",
            "mbif" => $"{Population[0]}",
            "mpp" => $"{Population[1]}",
            "tmax" => $"({string.Join(", ", Tmax)})",
            "tbif" => $"{Tmax[0]}",
            "tpp" => $"{Tmax[1]}",
            "criterion" => $"({string.Join(", ", Criterion)})",
            "cbif" => $"{Criterion[0]}",
            "cpp" => $"{Criterion[1]}",
            // "openmp" => $"{OpenMP} {(OpenMP ? $"(max_threads: {Parallelism})" : "")}",
            "openmp" => $"{string.Join(", ", Parallelism)}",
            _ => "e_Could not find the specified parameter."
        };
    }
}