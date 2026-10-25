package com.sde.station;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Color;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.Window;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;

/**
 * Coquille de l'appli SDE pour Android.
 *
 * La page (la même que celle servie par la station) tourne à une adresse fixe,
 * http://appli.sde/, servie par l'appli elle-même : les réglages gardés par la
 * page (localStorage) ne dépendent donc pas du réseau, et la page parle à la
 * station directement (192.168.4.1, son adresse à la maison ou l'accès à
 * distance). Quand la station annonce un autre firmware, l'appli télécharge sa
 * page et l'utilise : les mises à jour arrivent avec celles du firmware.
 */
public class MainActivity extends Activity {
    static final String HOST = "appli.sde";
    static final String START = "http://" + HOST + "/?app=1";
    static final String MARK = "SDEApk";   // la page sait tourner dans l'appli
    static final int REQ_FILE = 1, REQ_SAVE = 2;

    WebView web;
    SharedPreferences prefs;
    ConnectivityManager cm;
    final Handler ui = new Handler(Looper.getMainLooper());
    ValueCallback<Uri[]> fileCb;
    String saveText, saveUrl;
    volatile boolean updating;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        prefs = getSharedPreferences("sde", MODE_PRIVATE);
        cm = (ConnectivityManager) getSystemService(CONNECTIVITY_SERVICE);
        preferWifi();

        boolean dark = (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;
        web = new WebView(this);
        web.setBackgroundColor(dark ? 0xFF101010 : 0xFFF4F4F2);
        setBars(dark ? 0xFF1A1A19 : 0xFFFDFDFC, dark);
        setContentView(web);

        WebSettings ws = web.getSettings();
        ws.setJavaScriptEnabled(true);
        ws.setDomStorageEnabled(true);
        ws.setDatabaseEnabled(true);
        ws.setAllowFileAccess(false);
        ws.setUserAgentString(ws.getUserAgentString() + " SDEApk/" + BuildInfo.VERSION);
        web.addJavascriptInterface(new Bridge(), MARK);

        web.setWebViewClient(new WebViewClient() {
            @Override
            public WebResourceResponse shouldInterceptRequest(WebView v, WebResourceRequest r) {
                Uri u = r.getUrl();
                return HOST.equals(u.getHost()) ? serve(u.getPath()) : null;
            }

            @Override
            @SuppressWarnings("deprecation")
            public boolean shouldOverrideUrlLoading(WebView v, String url) {
                Uri u = Uri.parse(url);
                if (HOST.equals(u.getHost())) return false;
                // Un lien vers ailleurs (EMQX, GitHub...) s'ouvre dans le navigateur.
                try { startActivity(new Intent(Intent.ACTION_VIEW, u)); } catch (ActivityNotFoundException ignored) { }
                return true;
            }
        });
        web.setWebChromeClient(new WebChromeClient() {
            @Override
            public boolean onShowFileChooser(WebView v, ValueCallback<Uri[]> cb, FileChooserParams p) {
                if (fileCb != null) fileCb.onReceiveValue(null);
                fileCb = cb;
                Intent i = new Intent(Intent.ACTION_GET_CONTENT);
                i.addCategory(Intent.CATEGORY_OPENABLE);
                i.setType("*/*");
                try { startActivityForResult(Intent.createChooser(i, "Choisir un fichier"), REQ_FILE); }
                catch (ActivityNotFoundException e) { fileCb = null; return false; }
                return true;
            }
        });

        if (state != null) web.restoreState(state);
        else web.loadUrl(START);
    }

    // ------------------------------------------------------------ Page

    /** La page et ses icônes, servies depuis l'appli. */
    WebResourceResponse serve(String path) {
        if (path == null || path.equals("/") || path.isEmpty()) {
            try { return resp("text/html", 200, pageStream()); } catch (IOException e) { return notFound(); }
        }
        String name = path.substring(1);
        if (name.matches("[a-z0-9-]+\\.(png|webmanifest)")) {
            try { return resp(name.endsWith(".png") ? "image/png" : "application/manifest+json", 200, getAssets().open(name)); }
            catch (IOException ignored) { }
        }
        return notFound();   // en particulier /api/... : la station n'est pas ici
    }

    InputStream pageStream() throws IOException {
        File f = new File(getFilesDir(), "page.html");
        return f.exists() ? new FileInputStream(f) : getAssets().open("page.html");
    }

    String pageFw() {
        return prefs.getString("page_fw", BuildInfo.PAGE_FW);
    }

    static WebResourceResponse resp(String mime, int code, InputStream in) {
        Map<String, String> h = new HashMap<>();
        h.put("Cache-Control", "no-store");
        return new WebResourceResponse(mime, "utf-8", code, code == 200 ? "OK" : "Not Found", h, in);
    }

    static WebResourceResponse notFound() {
        return resp("text/plain", 404, new ByteArrayInputStream(new byte[0]));
    }

    /** La station a un autre firmware : on prend sa page, si elle sait tourner dans l'appli. */
    void fetchPage(final String base, final String fw) {
        if (updating) return;
        updating = true;
        new Thread(new Runnable() {
            public void run() {
                try {
                    byte[] page = get(base + "/", 30000);
                    String text = new String(page, "UTF-8");
                    if (page.length > 50000 && text.contains("<html") && text.contains(MARK)) {
                        File tmp = new File(getFilesDir(), "page.tmp");
                        try (OutputStream o = new FileOutputStream(tmp)) { o.write(page); }
                        if (!tmp.renameTo(new File(getFilesDir(), "page.html"))) throw new IOException("rename");
                        prefs.edit().putString("page_fw", fw).apply();
                        ui.post(new Runnable() { public void run() { web.reload(); } });
                    } else {
                        // Firmware trop ancien pour l'appli : on garde la page actuelle.
                        prefs.edit().putString("page_fw", fw).apply();
                    }
                } catch (IOException ignored) {
                    // Réessayé à la prochaine lecture de /api/info.
                } finally {
                    updating = false;
                }
            }
        }).start();
    }

    static byte[] get(String url, int timeout) throws IOException {
        HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
        c.setConnectTimeout(6000);
        c.setReadTimeout(timeout);
        try (InputStream in = c.getInputStream()) {
            if (c.getResponseCode() != 200) throw new IOException("HTTP " + c.getResponseCode());
            ByteArrayOutputStream b = new ByteArrayOutputStream();
            byte[] buf = new byte[16384];
            for (int n; (n = in.read(buf)) > 0; ) b.write(buf, 0, n);
            return b.toByteArray();
        } finally {
            c.disconnect();
        }
    }

    // ------------------------------------------------------------ Pont JavaScript

    class Bridge {
        /** Lecture de /api/info réussie sur place. */
        @JavascriptInterface
        public void seen(String base, String fw) {
            if (base != null && base.startsWith("http://") && fw != null && !fw.equals(pageFw())) fetchPage(base, fw);
        }

        /** Enregistrer un texte (export CSV) : l'utilisateur choisit où. */
        @JavascriptInterface
        public void save(final String name, String text) {
            saveText = text;
            saveUrl = null;
            ui.post(new Runnable() { public void run() { askWhere(name); } });
        }

        /** Enregistrer un fichier de la station (historique complet). */
        @JavascriptInterface
        public void download(String url, final String name) {
            saveText = null;
            saveUrl = url;
            ui.post(new Runnable() { public void run() { askWhere(name); } });
        }

        /** Couleur des barres du système, suivant le thème de la page. */
        @JavascriptInterface
        public void bar(final String color, final boolean dark) {
            ui.post(new Runnable() {
                public void run() {
                    try { setBars(Color.parseColor(color.trim()), dark); } catch (IllegalArgumentException ignored) { }
                }
            });
        }
    }

    void askWhere(String name) {
        Intent i = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("text/csv");
        i.putExtra(Intent.EXTRA_TITLE, name);
        try { startActivityForResult(i, REQ_SAVE); }
        catch (ActivityNotFoundException e) { toast("Enregistrement impossible sur cet appareil."); }
    }

    void writeTo(final Uri uri) {
        final String text = saveText, url = saveUrl;
        new Thread(new Runnable() {
            public void run() {
                try (OutputStream o = getContentResolver().openOutputStream(uri)) {
                    if (o == null) throw new IOException("flux");
                    o.write(text != null ? text.getBytes("UTF-8") : get(url, 120000));
                    toast("Fichier enregistré.");
                } catch (IOException e) {
                    toast("Échec de l'enregistrement : " + e.getMessage());
                }
            }
        }).start();
    }

    void toast(final String text) {
        ui.post(new Runnable() { public void run() { Toast.makeText(MainActivity.this, text, Toast.LENGTH_LONG).show(); } });
    }

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        if (req == REQ_FILE) {
            if (fileCb != null) fileCb.onReceiveValue(WebChromeClient.FileChooserParams.parseResult(res, data));
            fileCb = null;
        } else if (req == REQ_SAVE && res == RESULT_OK && data != null && data.getData() != null) {
            writeTo(data.getData());
        }
    }

    // ------------------------------------------------------------ Système

    /**
     * Le Wi-Fi SDE-XXXX n'a pas Internet : sans ça, Android enverrait les
     * requêtes par les données mobiles et ne trouverait jamais 192.168.4.1.
     * Tant qu'un Wi-Fi est là, l'appli passe par lui.
     */
    void preferWifi() {
        NetworkRequest req = new NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .build();
        cm.registerNetworkCallback(req, new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network n) { cm.bindProcessToNetwork(n); }

            @Override
            public void onLost(Network n) {
                if (n.equals(cm.getBoundNetworkForProcess())) cm.bindProcessToNetwork(null);
            }
        });
    }

    void setBars(int color, boolean dark) {
        Window w = getWindow();
        w.setStatusBarColor(color);
        w.setNavigationBarColor(color);
        View d = w.getDecorView();
        int f = d.getSystemUiVisibility();
        int light = View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR | (Build.VERSION.SDK_INT >= 26 ? 0x10 : 0);   // 0x10 : barre de navigation claire (API 26)
        f = dark ? f & ~light : f | light;
        d.setSystemUiVisibility(f);
    }

    @Override
    public void onBackPressed() {
        web.evaluateJavascript("window.sdeBack ? sdeBack() : false", new ValueCallback<String>() {
            public void onReceiveValue(String used) {
                if (!"true".equals(used)) MainActivity.super.onBackPressed();
            }
        });
    }

    @Override
    protected void onSaveInstanceState(Bundle out) {
        super.onSaveInstanceState(out);
        web.saveState(out);
    }

    @Override
    protected void onResume() {
        super.onResume();
        web.onResume();
    }

    @Override
    protected void onPause() {
        web.onPause();
        super.onPause();
    }
}
