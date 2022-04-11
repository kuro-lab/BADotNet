public static partial class StringExtensions
{
    public static bool IsAnyOf(this string self, params string[] values)
    {
        return values.Any(s => s == self);
    }
}