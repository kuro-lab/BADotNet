using System.Text;

class Program
{
    static void Main(string[] args)
    {
        CreateSample();

        Application app;

        try
        {
            if (args.Length == 0)
            {
                throw new ArgumentNullException();
            }
            app = new Application(args);
        }
        catch (InvalidDataException)
        {
            Application.PutError("The C source must have definitions for LMAX[], LMIN[], XMAX[], and XMIN[].");
            return;
        }
        catch (ArgumentException)
        {
            Application.PutError("Specify the .xml or .c file as the command-line argument.");
            return;
        }
        catch (FileNotFoundException)
        {
            Application.PutError($"Could not find the file \"{args[0]}\".");
            return;
        }

        while (true)
        {
            Console.Write(">> ");

            var input = Console.ReadLine();

            if (input == "" || input == null)
            {
                continue;
            }

            if (input.IsAnyOf("q", "quit", "exit"))
            {
                return;
            }

            string message = app.Command(input);

            if (message.Length > 0)
            {
                if (message.StartsWith("w_"))
                {
                    Application.PutWarning(message.Substring(2));
                }
                else if (message.StartsWith("e_"))
                {
                    Application.PutError(message.Substring(2));
                }
                else
                {
                    Application.Put(message);
                }
            }
        }
    }

    static void CreateSample()
    {
        if (File.Exists("Sample.c"))
        {
            return;
        }

        var sb = new StringBuilder();
        sb.AppendLine("// You can only use the statements #include<***.h>, NOT #include \" * *.h\"");
        sb.AppendLine("#include <math.h>");
        sb.AppendLine("");
        sb.AppendLine("// Upper and lower bounds of the search space for system parameters");
        sb.AppendLine("const double LMAX[] = {0.65, 2.0};");
        sb.AppendLine("const double LMIN[] = {0.35, 0.5};");
        sb.AppendLine("");
        sb.AppendLine("// for state variables");
        sb.AppendLine("const double XMAX[] = {1.0};");
        sb.AppendLine("const double XMIN[] = {0.0};");
        sb.AppendLine("");
        sb.AppendLine("// Evolution functions of the system");
        sb.AppendLine("// Updates the values of the state variables array x with the parameters array l.");
        sb.AppendLine("void next_x(double *x, double *l) {");
        sb.AppendLine("    x[0] = x[0] + l[0] - l[1] / 2.0 / M_PI * sin(2.0 * M_PI * x[0]);");
        sb.AppendLine("    x[0] -= (int)x[0];");
        sb.AppendLine("    return;");
        sb.AppendLine("}");

        using (var writer = new StreamWriter("Sample.c"))
        {
            writer.WriteLine(sb.ToString());
        }

        return;
    }
}