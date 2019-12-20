package com.hong.mymusic;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.Fragment;
import android.support.v4.content.ContextCompat;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import java.io.File;
import java.util.ArrayList;
import java.util.List;


public class SourceFragment extends Fragment {

    private SourceListAdapter sourceListAdapter;
    private RecyclerView recyclerView;
    private List<SourceListBean> datas;
    private int count = 0;
    private List<String> paths;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_source,container,false);
        recyclerView = view.findViewById(R.id.recycler_view);
        return view;
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        datas = new ArrayList<>();
        paths = new ArrayList<>();

        sourceListAdapter = new SourceListAdapter(getActivity(), datas);
        LinearLayoutManager layoutManager = new LinearLayoutManager(getActivity());
        layoutManager.setOrientation(LinearLayoutManager.VERTICAL);
        recyclerView.setLayoutManager(layoutManager);
        recyclerView.setAdapter(sourceListAdapter);

        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (ContextCompat.checkSelfPermission(getActivity(), Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED//读取存储卡权限
                    || ContextCompat.checkSelfPermission(getActivity(), Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(getActivity(), new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0x1001);
            } else {
                initData();
            }
        } else {
            initData();
        }

        sourceListAdapter.setOnItemClickListener(new SourceListAdapter.OnItemClickListener() {
            @Override
            public void onItemClick(SourceListBean localListBean) {
                if(localListBean != null) {
                    if(!localListBean.isFile()) {
                        List<SourceListBean> d = getDirFiles(localListBean.getPath());
                        if(d.size() > 0) {
                            paths.add(localListBean.getParent());
                            count++;
                            datas.clear();
                            datas.addAll(d);
                            sourceListAdapter.notifyDataSetChanged();
                            System.out.println("count:" + count);
                        } else {
                            Toast.makeText(getActivity(), "没有下级目录了", Toast.LENGTH_SHORT).show();
                        }
                    } else {
                        //开始播放
                        ((MainActivity)getActivity()).setDataSource(localListBean.getPath());
                    }

                }
            }
        });
    }

    private void initData() {
        datas.clear();
        paths.clear();
        File file = Environment.getExternalStorageDirectory().getAbsoluteFile();
        paths.add(file.getAbsolutePath());
        File[] files = file.listFiles();
        for(int i = 0; i < files.length; i++) {
            SourceListBean sourceListBean = new SourceListBean();
            sourceListBean.setParent(file.getAbsolutePath());
            sourceListBean.setName(files[i].getName());
            sourceListBean.setPath(files[i].getAbsolutePath());
            sourceListBean.setFile(files[i].isFile());

            String name = sourceListBean.getName();
            if(name.endsWith("mp3")
            || name.endsWith("ape")
            || name.endsWith("flac")
            || name.endsWith("m3u8"))
            datas.add(sourceListBean);
        }

        //加入几个在线的音频播放地址
        SourceListBean bean0 = new SourceListBean();
        bean0.setName("在线mp3");
        bean0.setPath("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
        bean0.setFile(true);
        datas.add(0,bean0);

        SourceListBean bean1 = new SourceListBean();
        bean1.setName("在线m3u8");
        bean1.setPath("http://ngcdn004.cnr.cn/live/dszs/index.m3u8");
        bean1.setFile(true);
        datas.add(1,bean1);

        sourceListAdapter.notifyDataSetChanged();
    }

    private List<SourceListBean> getDirFiles(String path) {
        List<SourceListBean> sources = new ArrayList<>();
        File file = new File(path);
        if(file.exists()) {
            File[] files = file.listFiles();
            if(files != null && files.length > 0) {
                for(int i = 0; i < files.length; i++) {
                    SourceListBean sourceListBean = new SourceListBean();
                    sourceListBean.setFile(!files[i].isDirectory());
                    sourceListBean.setPath(files[i].getAbsolutePath());
                    sourceListBean.setName(files[i].getName());
                    sourceListBean.setParent(path);
                    sources.add(sourceListBean);
                }

            }
        }
        return sources;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            initData();
        } else {
            Toast.makeText(getActivity(), "请允许读取存储卡权限", Toast.LENGTH_SHORT).show();
        }
    }

}
