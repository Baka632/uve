﻿#pragma checksum "D:\Projects\UVEngine\UVEngine\System\DonatePage.xaml" "{406ea660-64cf-4c82-b6f0-42d48172a799}" "B08FF8EA84E4A96C523F542B474F6F33"
//------------------------------------------------------------------------------
// <auto-generated>
//     此代码由工具生成。
//     运行时版本:4.0.30319.42000
//
//     对此文件的更改可能会导致不正确的行为，并且如果
//     重新生成代码，这些更改将会丢失。
// </auto-generated>
//------------------------------------------------------------------------------

using Microsoft.Phone.Controls;
using System;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Automation.Peers;
using System.Windows.Automation.Provider;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Resources;
using System.Windows.Shapes;
using System.Windows.Threading;


namespace UVEngine {
    
    
    public partial class DonatePage : Microsoft.Phone.Controls.PhoneApplicationPage {
        
        internal System.Windows.Controls.Grid LayoutRoot;
        
        internal System.Windows.Controls.Grid ContentPanel;
        
        internal System.Windows.Controls.CheckBox nomore;
        
        internal System.Windows.Controls.Grid Buttons;
        
        internal System.Windows.Controls.Button donate;
        
        internal System.Windows.Controls.Button @continue;
        
        private bool _contentLoaded;
        
        /// <summary>
        /// InitializeComponent
        /// </summary>
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        public void InitializeComponent() {
            if (_contentLoaded) {
                return;
            }
            _contentLoaded = true;
            System.Windows.Application.LoadComponent(this, new System.Uri("/UVEngine;component/System/DonatePage.xaml", System.UriKind.Relative));
            this.LayoutRoot = ((System.Windows.Controls.Grid)(this.FindName("LayoutRoot")));
            this.ContentPanel = ((System.Windows.Controls.Grid)(this.FindName("ContentPanel")));
            this.nomore = ((System.Windows.Controls.CheckBox)(this.FindName("nomore")));
            this.Buttons = ((System.Windows.Controls.Grid)(this.FindName("Buttons")));
            this.donate = ((System.Windows.Controls.Button)(this.FindName("donate")));
            this.@continue = ((System.Windows.Controls.Button)(this.FindName("continue")));
        }
    }
}

