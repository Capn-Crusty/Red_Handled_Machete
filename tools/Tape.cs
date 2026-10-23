using System; using System.Collections.Generic; using System.Drawing; using System.Drawing.Imaging; using System.Runtime.InteropServices;

public static class Tape {
  static byte[] Load(string f, out int w, out int h) {
    using (var b0 = new Bitmap(f)) {
      w = b0.Width; h = b0.Height; var r = new Rectangle(0,0,w,h);
      using (var b = b0.Clone(r, PixelFormat.Format32bppArgb)) {
        var d = b.LockBits(r, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
        var p = new byte[d.Stride*h]; Marshal.Copy(d.Scan0,p,0,p.Length); b.UnlockBits(d); return p; } } }
  static void Save(byte[] p, int w, int h, string f) {
    using (var b = new Bitmap(w,h,PixelFormat.Format32bppArgb)) { var r = new Rectangle(0,0,w,h);
      var d = b.LockBits(r, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb); Marshal.Copy(p,0,d.Scan0,p.Length); b.UnlockBits(d); b.Save(f, ImageFormat.Png); } }
  static double Hash(int x, int y) { uint n = (uint)(x*374761393 + y*668265263); n = (n ^ (n >> 13)) * 1274126177u; return ((n ^ (n >> 16)) & 0xffff) / 65535.0; }
  static double VNoise(double x, double y) { int xi=(int)Math.Floor(x), yi=(int)Math.Floor(y); double fx=x-xi, fy=y-yi;
    fx=fx*fx*(3-2*fx); fy=fy*fy*(3-2*fy);
    double a=Hash(xi,yi), b=Hash(xi+1,yi), c=Hash(xi,yi+1), d=Hash(xi+1,yi+1);
    return a+(b-a)*fx+(c-a)*fy+(a-b-c+d)*fx*fy; }
  static double Fbm(double x, double y) { return 0.5*VNoise(x,y)+0.3*VNoise(x*2.1+17,y*2.1+5)+0.2*VNoise(x*4.3+3,y*4.3+11); }
  static double Clamp(double v, double a, double b) { return v<a?a:(v>b?b:v); }

  // bands: tape wraps per island; slope: diagonal lean of the wraps (dx per dy)
  public static string Run(string dir, int bands, double slope) {
    int w,h; var co = Load(dir+"\\machete_co.png", out w, out h);
    var smdi = Load(dir+"\\smdi.png", out w, out h); var nohq = Load(dir+"\\nohq.png", out w, out h);
    int n = w*h; var mask = new bool[n];
    for (int i=0;i<n;i++) { double B=co[i*4]/255.0, G=co[i*4+1]/255.0, R=co[i*4+2]/255.0;
      double mx=Math.Max(R,Math.Max(G,B)), mn=Math.Min(R,Math.Min(G,B)), c=mx-mn;
      mask[i] = c>=0.04 && mx==G && G>R+0.02; }
    // handle pieces (two rounded strips + grip face); anything non-black inside them is handle
    var boxes = new List<int[]>{ new int[]{33,338,1565,481,999999}, new int[]{34,482,2034,612,999999}, new int[]{29,617,919,944,999999} };
    var lab = new int[n]; var hole = new bool[n];
    for (int i=0;i<n;i++){ int x=i%w, y=i/w; double v=Math.Max(co[i*4+1],Math.Max(co[i*4],co[i*4+2]))/255.0;
      bool rivet = Math.Pow(x-282,2)+Math.Pow(y-758,2) < 46*46 || Math.Pow(x-625,2)+Math.Pow(y-742,2) < 46*46;
      if (v < 0.03 && !rivet) continue;
      for (int k=0;k<3;k++){ var b=boxes[k]; if (x>=b[0]&&x<=b[2]&&y>=b[1]&&y<=b[3]) { lab[i]=k+1; break; } }
      if (lab[i]==0) continue;
      if (lab[i]==3 && rivet) hole[i]=true;                      // rivets are covered by the tape
      else if (lab[i]==3 && y >= 928 && !mask[i]) lab[i]=0;      // the pin wire under the grip face is left alone
      else if (!mask[i] && v > 0.36) hole[i]=true; }             // light wear specks: tape, flat shading
    // median brightness of the original handle, for keeping its shading/AO
    var vals = new List<double>(); for (int i=0;i<n;i+=7) if(mask[i]) vals.Add(Math.Max(co[i*4+1],Math.Max(co[i*4],co[i*4+2]))/255.0);
    vals.Sort(); double med = vals[vals.Count/2];
    string info = "median "+med.ToString("F3")+"\n";
    foreach (var b in boxes) info += string.Format("island x{0}-{2} y{1}-{3} px{4}\n", b[0],b[1],b[2],b[3],b[4]);

    double baseR=0.50, baseG=0.055, baseB=0.065; // athletic-tape red
    for (int y=0;y<h;y++) for (int x=0;x<w;x++) { int i=y*w+x; int L=lab[i]; if (L==0) continue;
      var bx = boxes[L-1]; if (bx[4] < 2000) continue; // ignore specks
      double W = bx[2]-bx[0]+1, S = W/bands, dl = Math.Sqrt(1+slope*slope);
      double u = (-(x-bx[0])*slope + (y-bx[1]))/dl;          // along the tape
      double v = ((x-bx[0]) + (y-bx[1])*slope)/dl;           // across the tape
      double p = v*dl/S; int band=(int)Math.Floor(p);
      double t = p-band + 0.03*(VNoise(u/6.0, band*13.0)-0.5); // slightly ragged tape edges
      if (t < 0) { t += 1; band--; } else if (t >= 1) { t -= 1; band++; }
      // brightness across one wrap: lit raised lip at the start, deepening shadow before the next wrap
      double br = 1.05 - 0.10*t;
      if (t < 0.05) br += 0.14*(1-t/0.05);
      if (t > 0.84) br -= 0.50*Math.Pow((t-0.84)/0.16, 1.4);
      br *= 0.95 + 0.10*Hash(band, L*31);                   // wrap-to-wrap tone variation
      // athletic-tape plain weave: threads along and across the tape, alternating over/under
      const double P = 3.4; double fu=u/P, fv=v/P; int cu=(int)Math.Floor(fu), cv=(int)Math.Floor(fv);
      bool warp = ((cu+cv)&1)==0; double ph = warp ? fv-cv : fu-cu; double th = Math.Sin(Math.PI*ph);
      br *= 0.80 + 0.30*th + 0.14*(Hash(x,y)-0.5);
      br *= 1 + 0.08*(VNoise(u/1.5, v/12.0)-0.5);            // fibre streaks
      // grime: blotches, dirt packed into the weave and along the wrap edges, finer smudges
      double g = Fbm(x/48.0, y/48.0); double grime = Clamp((g-0.32)*1.7,0,1);
      double smudge = Clamp((Fbm(x/14.0+90, y/14.0+30)-0.5)*2.2,0,1);
      double edge = t > 0.80 ? (t-0.80)/0.20 : (t < 0.06 ? 0.5*(1-t/0.06) : 0);
      grime = Clamp(0.65*grime + 0.35*smudge + 0.45*edge + 0.25*(1-th), 0, 1);
      double wear = Clamp((Fbm(x/9.0+50, y/9.0+50)-0.72)*4,0,1);
      double shade = 0.93;
      if (!hole[i]) { double vv = Math.Max(co[i*4+1],Math.Max(co[i*4],co[i*4+2]))/255.0; shade = Math.Pow(Clamp(vv/med,0.35,1.35),0.75); }
      double r=baseR*br*shade, gg=baseG*br*shade, bb=baseB*br*shade;
      double dirt = 0.62*grime; r = r*(1-dirt)+0.13*dirt*shade; gg = gg*(1-dirt)+0.09*dirt*shade; bb = bb*(1-dirt)+0.06*dirt*shade;
      r = r*(1-0.35*wear)+0.55*0.35*wear*shade; gg = gg*(1-0.35*wear)+0.40*0.35*wear*shade; bb = bb*(1-0.35*wear)+0.38*0.35*wear*shade; // frayed lighter fibres
      co[i*4+2]=(byte)(Clamp(r,0,1)*255); co[i*4+1]=(byte)(Clamp(gg,0,1)*255); co[i*4]=(byte)(Clamp(bb,0,1)*255);
      // matte cloth: almost no specular (G) or gloss (B)
      smdi[i*4+1]=(byte)(4+4*Hash(x+7,y)); smdi[i*4]=(byte)(10+8*grime);
      // normal: original shape (flat over rivets) + wrap lip/shadow slope + thread bumps
      double nx=0, ny=0;
      if (!hole[i]) { nx=(nohq[i*4+2]-128)/127.0; ny=(nohq[i*4+1]-128)/127.0; }
      double ridge = 0; if (t < 0.05) ridge = 0.70; else if (t > 0.90) ridge = -0.35*(t-0.90)/0.10;
      double dvs = 0.22*Math.Cos(Math.PI*ph);                // thread slope, across (warp) or along (weft)
      double ga = warp ? 0 : dvs, gd = ridge + (warp ? dvs : 0);
      nx += -(gd*(1/dl) + ga*(-slope/dl)); ny += -(gd*(slope/dl) + ga*(1/dl));
      double nz = Math.Sqrt(Math.Max(0.05, 1-nx*nx-ny*ny)); double l=Math.Sqrt(nx*nx+ny*ny+nz*nz);
      nohq[i*4+2]=(byte)Clamp(128+127*nx/l,0,255); nohq[i*4+1]=(byte)Clamp(128+127*ny/l,0,255); nohq[i*4]=(byte)Clamp(128+127*nz/l,0,255);
    }
    Save(co,w,h,dir+"\\tape_co.png"); Save(smdi,w,h,dir+"\\tape_smdi.png"); Save(nohq,w,h,dir+"\\tape_nohq.png");
    return info;
  }

  // Writes machete_red.rvmat, _damage and _destruct into outDir: vanilla's three machete materials
  // (from vanillaDataDir, e.g. P:\DZ\weapons\melee\blade\data) pointing at our normal and specular maps.
  public static string Materials(string vanillaDataDir, string outDir) {
    string[] names = { "machete", "machete_damage", "machete_destruct" };
    foreach (var n in names) {
      string text = System.IO.File.ReadAllText(System.IO.Path.Combine(vanillaDataDir, n + ".rvmat"));
      text = System.Text.RegularExpressions.Regex.Replace(text, @"dz\\weapons\\melee\\blade\\data\\machete_(nohq|smdi)\.paa",
        m => "RedHandleMachete\\data\\machete_red_" + m.Groups[1].Value + ".paa", System.Text.RegularExpressions.RegexOptions.IgnoreCase);
      System.IO.File.WriteAllText(System.IO.Path.Combine(outDir, n.Replace("machete", "machete_red") + ".rvmat"), text);
    }
    return "wrote 3 materials";
  }

  // Saw-back test:cuts see-through teeth (alpha 0) into the spine side of the blade face in tape_co.png,
  // from x0 to x1 (the blade face's top edge is the spine; the handle end is on the right).
  public static string Serrate(string dir, int x0, int x1, double pitch, int depth) {
    int w,h; var co = Load(dir+"\\tape_co.png", out w, out h); int cut=0;
    for (int x=x0; x<=x1; x++) {
      int top=-1; for (int y=0;y<335;y++){ int i=y*w+x; if (Math.Max(co[i*4],Math.Max(co[i*4+1],co[i*4+2])) > 12) { top=y; break; } }
      if (top<0) continue;
      double f = ((x-x0)/pitch) % 1.0; double tooth = f < 0.7 ? f/0.7 : (1-f)/0.3; // raked tooth profile
      int d = (int)Math.Round(depth*(1-tooth));
      for (int y=0; y<top+d; y++) { co[(y*w+x)*4+3]=0; cut++; }
    }
    Save(co,w,h,dir+"\\tape_co.png"); return "cut "+cut+" px";
  }

  // Removes the spine's top face (the long straight strip near the bottom of the texture) between sx0 and sx1,
  // so no solid line runs across the teeth. The strip is found by flood fill from (seedX, seedY).
  public static string OpenSpine(string dir, int seedX, int seedY, int sx0, int sx1) {
    int w,h; var co = Load(dir+"\\tape_co.png", out w, out h); var seen = new bool[w*h]; var st = new Stack<int>();
    int minx=w,maxx=0,miny=h,maxy=0,cut=0; st.Push(seedY*w+seedX);
    while (st.Count>0) { int i=st.Pop(); if (seen[i]) continue; seen[i]=true; int x=i%w, y=i/w;
      if (Math.Max(co[i*4],Math.Max(co[i*4+1],co[i*4+2])) <= 20 || y < 930) continue;
      if(x<minx)minx=x; if(x>maxx)maxx=x; if(y<miny)miny=y; if(y>maxy)maxy=y;
      if (x>=sx0 && x<=sx1) { co[i*4+3]=0; cut++; }
      if(x>0)st.Push(i-1); if(x<w-1)st.Push(i+1); if(y>0)st.Push(i-w); if(y<h-1)st.Push(i+w); }
    // also clear the dark anti-aliased rim around the cut stretch
    for (int y=miny-2;y<=maxy+2;y++) for (int x=sx0;x<=sx1;x++) { int i=y*w+x; if (!seen[i] && y<=maxy+2) {
      bool nb=false; for(int dy=-2;dy<=2&&!nb;dy++) for(int dx=-2;dx<=2&&!nb;dx++){ int j=(y+dy)*w+x+dx; if (seen[j] && co[j*4+3]==0) nb=true; }
      if (nb) co[i*4+3]=0; } }
    Save(co,w,h,dir+"\\tape_co.png");
    return string.Format("strip x{0}-{1} y{2}-{3}, cut {4} px", minx,maxx,miny,maxy,cut);
  }
}
