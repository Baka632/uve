﻿#pragma checksum "D:\Projects\UVEngine\UVEngine2_1\Controls\Hamburger\Hamburger.xaml" "{406ea660-64cf-4c82-b6f0-42d48172a799}" "E9ACC4D5186D270F9058E1FB4D75C53D"
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


namespace UVEngine2_1.Controls.Hamburger {
    
    
    public partial class Hamburger : System.Windows.Controls.UserControl {
        
        internal System.Windows.Controls.Grid HamburgerGrid;
        
        internal System.Windows.Shapes.Rectangle IconBackground;
        
        internal System.Windows.Shapes.Rectangle ListBackground;
        
        internal Microsoft.Phone.Controls.LongListSelector PageSelector;
        
        internal System.Windows.Controls.Button ExpandButton;
        
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
            System.Windows.Application.LoadComponent(this, new System.Uri("/UVEngine2_1;component/Controls/Hamburger/Hamburger.xaml", System.UriKind.Relative));
            this.HamburgerGrid = ((System.Windows.Controls.Grid)(this.FindName("HamburgerGrid")));
            this.IconBackground = ((System.Windows.Shapes.Rectangle)(this.FindName("IconBackground")));
            this.ListBackground = ((System.Windows.Shapes.Rectangle)(this.FindName("ListBackground")));
            this.PageSelector = ((Microsoft.Phone.Controls.LongListSelector)(this.FindName("PageSelector")));
            this.ExpandButton = ((System.Windows.Controls.Button)(this.FindName("ExpandButton")));
        }
    }
}

