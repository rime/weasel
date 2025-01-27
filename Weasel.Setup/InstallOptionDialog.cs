using System.Windows.Forms;

namespace Weasel.Setup
{
    public partial class InstallOptionDialog : Form
    {
        public bool IsInstalled { get; set; }
        public bool IsHant { get; set; }
        public string UserDir { get; set; }

        public InstallOptionDialog()
        {
            InitializeComponent();
        }
    }
}
