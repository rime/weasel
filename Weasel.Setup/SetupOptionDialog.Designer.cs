namespace Weasel.Setup
{
    partial class SetupOptionDialog
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要修改
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SetupOptionDialog));
            this.inputLangGroup = new System.Windows.Forms.GroupBox();
            this.chtRadio = new System.Windows.Forms.RadioButton();
            this.chsRadio = new System.Windows.Forms.RadioButton();
            this.userFolderGroup = new System.Windows.Forms.GroupBox();
            this.selectButton = new System.Windows.Forms.Button();
            this.customPathBox = new System.Windows.Forms.TextBox();
            this.customFolderRadio = new System.Windows.Forms.RadioButton();
            this.defaultFolderRadio = new System.Windows.Forms.RadioButton();
            this.confirmButton = new System.Windows.Forms.Button();
            this.removeButton = new System.Windows.Forms.Button();
            this.inputLangGroup.SuspendLayout();
            this.userFolderGroup.SuspendLayout();
            this.SuspendLayout();
            // 
            // inputLangGroup
            // 
            this.inputLangGroup.Controls.Add(this.chtRadio);
            this.inputLangGroup.Controls.Add(this.chsRadio);
            resources.ApplyResources(this.inputLangGroup, "inputLangGroup");
            this.inputLangGroup.Name = "inputLangGroup";
            this.inputLangGroup.TabStop = false;
            // 
            // chtRadio
            // 
            resources.ApplyResources(this.chtRadio, "chtRadio");
            this.chtRadio.Name = "chtRadio";
            this.chtRadio.TabStop = true;
            this.chtRadio.UseVisualStyleBackColor = true;
            // 
            // chsRadio
            // 
            resources.ApplyResources(this.chsRadio, "chsRadio");
            this.chsRadio.Name = "chsRadio";
            this.chsRadio.TabStop = true;
            this.chsRadio.UseVisualStyleBackColor = true;
            // 
            // userFolderGroup
            // 
            this.userFolderGroup.Controls.Add(this.selectButton);
            this.userFolderGroup.Controls.Add(this.customPathBox);
            this.userFolderGroup.Controls.Add(this.customFolderRadio);
            this.userFolderGroup.Controls.Add(this.defaultFolderRadio);
            resources.ApplyResources(this.userFolderGroup, "userFolderGroup");
            this.userFolderGroup.Name = "userFolderGroup";
            this.userFolderGroup.TabStop = false;
            // 
            // selectButton
            // 
            resources.ApplyResources(this.selectButton, "selectButton");
            this.selectButton.Name = "selectButton";
            this.selectButton.UseVisualStyleBackColor = true;
            this.selectButton.Click += new System.EventHandler(this.SelectButton_Click);
            // 
            // customPathBox
            // 
            resources.ApplyResources(this.customPathBox, "customPathBox");
            this.customPathBox.Name = "customPathBox";
            // 
            // customFolderRadio
            // 
            resources.ApplyResources(this.customFolderRadio, "customFolderRadio");
            this.customFolderRadio.Name = "customFolderRadio";
            this.customFolderRadio.TabStop = true;
            this.customFolderRadio.UseVisualStyleBackColor = true;
            this.customFolderRadio.CheckedChanged += new System.EventHandler(this.CustomFolderRadio_CheckedChanged);
            // 
            // defaultFolderRadio
            // 
            resources.ApplyResources(this.defaultFolderRadio, "defaultFolderRadio");
            this.defaultFolderRadio.Name = "defaultFolderRadio";
            this.defaultFolderRadio.TabStop = true;
            this.defaultFolderRadio.UseVisualStyleBackColor = true;
            this.defaultFolderRadio.CheckedChanged += new System.EventHandler(this.DefaultFolderRadio_CheckedChanged);
            // 
            // confirmButton
            // 
            resources.ApplyResources(this.confirmButton, "confirmButton");
            this.confirmButton.Name = "confirmButton";
            this.confirmButton.UseVisualStyleBackColor = true;
            this.confirmButton.Click += new System.EventHandler(this.ConfirmButton_Click);
            // 
            // removeButton
            // 
            resources.ApplyResources(this.removeButton, "removeButton");
            this.removeButton.Name = "removeButton";
            this.removeButton.UseVisualStyleBackColor = true;
            this.removeButton.Click += new System.EventHandler(this.RemoveButton_Click);
            // 
            // SetupOptionDialog
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.removeButton);
            this.Controls.Add(this.confirmButton);
            this.Controls.Add(this.userFolderGroup);
            this.Controls.Add(this.inputLangGroup);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "SetupOptionDialog";
            this.ShowIcon = false;
            this.Load += new System.EventHandler(this.InstallOptionDialog_Load);
            this.inputLangGroup.ResumeLayout(false);
            this.inputLangGroup.PerformLayout();
            this.userFolderGroup.ResumeLayout(false);
            this.userFolderGroup.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox inputLangGroup;
        private System.Windows.Forms.GroupBox userFolderGroup;
        private System.Windows.Forms.RadioButton chtRadio;
        private System.Windows.Forms.RadioButton chsRadio;
        private System.Windows.Forms.RadioButton customFolderRadio;
        private System.Windows.Forms.RadioButton defaultFolderRadio;
        private System.Windows.Forms.Button confirmButton;
        private System.Windows.Forms.Button removeButton;
        private System.Windows.Forms.TextBox customPathBox;
        private System.Windows.Forms.Button selectButton;
    }
}

