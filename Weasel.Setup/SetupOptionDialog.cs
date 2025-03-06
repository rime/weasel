using System.Windows.Forms;

namespace Weasel.Setup
{
    public partial class SetupOptionDialog : Form
    {
        public bool IsInstalled { get; set; }
        public bool IsHant { get; set; }
        public string UserDir { get; set; }

        public SetupOptionDialog()
        {
            InitializeComponent();
        }

        private void InstallOptionDialog_Load(object sender, System.EventArgs e)
        {
            chsRadio.Checked = !IsHant;
            chtRadio.Checked = IsHant;

            inputLangGroup.Enabled = !IsInstalled;

            defaultFolderRadio.Checked = string.IsNullOrEmpty(UserDir);
            customFolderRadio.Checked = !string.IsNullOrEmpty(UserDir);
            customPathBox.Enabled = !string.IsNullOrEmpty(UserDir);
            selectButton.Enabled = !string.IsNullOrEmpty(UserDir);
            customPathBox.Text = UserDir;

            if (IsInstalled)
            {
                confirmButton.Text = Localization.Resources.STR_OK;
            }
            removeButton.Enabled = IsInstalled;
        }

        private void DefaultFolderRadio_CheckedChanged(object sender, System.EventArgs e)
        {
            if (defaultFolderRadio.Checked)
            {
                customPathBox.Text = string.Empty;
                customPathBox.Enabled = false;
                selectButton.Enabled = false;
            }
        }

        private void CustomFolderRadio_CheckedChanged(object sender, System.EventArgs e)
        {
            if (customFolderRadio.Checked)
            {
                customPathBox.Enabled = true;
                selectButton.Enabled = true;
            }
        }

        private void ConfirmButton_Click(object sender, System.EventArgs e)
        {
            IsHant = chtRadio.Checked;
            UserDir = customFolderRadio.Checked
                ? customPathBox.Text
                : string.Empty;
            DialogResult = DialogResult.OK;
            Close();
        }

        private void SelectButton_Click(object sender, System.EventArgs e)
        {
            var folderDialog = new FolderBrowserDialog();
            var customPath = customPathBox.Text;
            if (!string.IsNullOrEmpty(customPath))
            {
                folderDialog.SelectedPath = customPath;
            }
            if (DialogResult.OK == folderDialog.ShowDialog())
            {
                customPathBox.Text = folderDialog.SelectedPath;
            }
            confirmButton.Focus();
        }

        private void RemoveButton_Click(object sender, System.EventArgs e)
        {
            Setup.Uninstall(false);
            IsInstalled = false;
            confirmButton.Text = Localization.Resources.STR_INSTALL;
            inputLangGroup.Enabled = !IsInstalled;
            removeButton.Enabled = IsInstalled;
        }
    }
}
